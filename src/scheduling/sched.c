#include "sched.h"
#include "../gdt.h"
#include "../process.h"
#include "../memory/vmm.h"
#include "../trace.h"

static task_t *g_runq = NULL;        // circular list 
static task_t *g_current = NULL;     // currently running task
static uint32_t g_next_pid = 1;

static uint32_t g_quantum = 5;       // in timer ticks
static uint32_t g_left = 5;

static int g_enabled = 0;

static void runq_push(task_t *t) {
    if (g_runq == NULL) {
        g_runq = t;
        t->next_task = t;
        return;
    }
    
    task_t *tail = g_runq;
    while (tail->next_task != g_runq) tail = tail->next_task;
    tail->next_task = t;
    t->next_task = g_runq;
}

static void runq_remove(task_t *t) {
    if (g_runq == NULL || t == NULL) {
        return;
    }
    if (g_runq == t && g_runq->next_task == g_runq) {
        g_runq = NULL;
        t->next_task = NULL;
        return;
    }
    task_t *prev = g_runq;
    while (prev->next_task != t && prev->next_task != g_runq) {
        prev = prev->next_task;
    }
    if (prev->next_task != t) {
        return;
    }
    prev->next_task = t->next_task;
    if (g_runq == t) {
        g_runq = t->next_task;
    }
    t->next_task = NULL;
}

static task_t* runq_next_runnable(task_t *from) {
    if (from == NULL) {
        return NULL;
    }
    task_t *t = from->next_task;
    while (t != from) {
        if (t->state == TASK_READY || t->state == TASK_RUNNING) {
            return t;
        }
        t = t->next_task;
    }
    if (from->state == TASK_READY || from->state == TASK_RUNNING) {
        return from;
    }
    return NULL;
}

// We treat the kmain as an idle task the first time the scheduler runs.
// That way we can always switch back if needed.
static void ensure_idle_task(uint32_t current_saved_esp) {
    if (g_current != NULL) {
        return;
    }

    task_t *idle = kmalloc(sizeof(task_t));
    memset(idle, 0, sizeof(*idle));
    idle->pid = 0;
    idle->state = TASK_RUNNING;
    idle->saved_esp = current_saved_esp;
    idle->stack_base = NULL;
    idle->stack_size = 0;
    idle->next_task = NULL;
    idle->page_dir_phys = vmm_kernel_pd_phys();

    g_current = idle;
    runq_push(idle);
}

void sched_init(uint32_t quantum_ticks) {
    if (quantum_ticks == 0) {
        g_quantum = 1;
    } else {
        g_quantum = quantum_ticks;
    }
    g_left = g_quantum;
    g_enabled = 0;
    g_runq = NULL;
    g_current = NULL;
    g_next_pid = 1;
}

void sched_enable(int enabled) {
    if (enabled != 0) {
        g_enabled = 1;
    } else {
        g_enabled = 0;
    }
}

task_t *sched_current(void) {
    return g_current;
}


// Build an initial task_frame on the new task's stack so that when the stub restores
// regs + iretd, it lands at entry().
task_t* task_create(void (*entry)(void), uint32_t stack_size) {
    if (stack_size == 0) {
        stack_size = STACK_SIZE_DEFAULT;
    }

    task_t *t = kmalloc(sizeof(task_t));
    if (t == NULL) {
        return NULL;
    }
    memset(t, 0, sizeof(*t));

    t->pid = g_next_pid++;
    t->state = TASK_READY;
    t->stack_size = stack_size;
    t->stack_base = kmalloc(stack_size);
    if (t->stack_base == NULL) {
        print_error("task_create: stack alloc failed\n");
        return NULL;
    }
    t->kstack_base = t->stack_base;
    t->kstack_size = t->stack_size;
    t->kstack_top = (uint32_t)t->stack_base + t->stack_size;
    t->page_dir_phys = vmm_kernel_pd_phys();
    t->is_user = 0;
    t->proc = NULL;

    uint32_t top = (uint32_t)t->stack_base + t->stack_size;

    // Place task_frame_t at the top of the stack.
    task_frame_t *f = (task_frame_t *)(top - sizeof(task_frame_t));
    memset(f, 0, sizeof(*f));

    // Segment registers (kernel mode)
    f->cs = KERNEL_CS;
    f->ds = f->es = f->fs = f->gs = KERNEL_DS;

    // Interrupt flag on (so preemption keeps working)
    f->eflags = 0x202;

    // Start executing here
    f->eip = (uint32_t)entry;

    // Not strictly important for your restore path, but keep sane values:
    f->int_no = 0;
    f->err_code = 0;

    // saved_esp points to gs (start of task_frame_t)
    t->saved_esp = (uint32_t)f;

    runq_push(t);
    return t;
}

task_t* task_create_user(uint32_t entry, uint32_t user_stack_top) {
    task_t *t = kmalloc(sizeof(task_t));
    if (t == NULL) {
        return NULL;
    }
    memset(t, 0, sizeof(*t));

    t->pid = g_next_pid++;
    t->state = TASK_READY;
    t->is_user = 1;
    t->proc = NULL;
    t->page_dir_phys = vmm_kernel_pd_phys();

    t->kstack_size = STACK_SIZE_DEFAULT;
    t->kstack_base = kmalloc(t->kstack_size);
    if (t->kstack_base == NULL) {
        print_error("task_create_user: kstack alloc failed\n");
        return NULL;
    }
    t->kstack_top = (uint32_t)t->kstack_base + t->kstack_size;

    uint32_t *stk = (uint32_t *)t->kstack_top;

    *(--stk) = USER_DS;          // ss
    *(--stk) = user_stack_top;   // user esp
    *(--stk) = 0x202;            // eflags
    *(--stk) = USER_CS;          // cs
    *(--stk) = entry;            // eip
    *(--stk) = 0;                // err_code
    *(--stk) = 0;                // int_no

    *(--stk) = 0;                // eax
    *(--stk) = 0;                // ecx
    *(--stk) = 0;                // edx
    *(--stk) = 0;                // ebx
    *(--stk) = 0;                // esp
    *(--stk) = 0;                // ebp
    *(--stk) = 0;                // esi
    *(--stk) = 0;                // edi

    *(--stk) = USER_DS;          // ds
    *(--stk) = USER_DS;          // es
    *(--stk) = USER_DS;          // fs
    *(--stk) = USER_DS;          // gs

    t->saved_esp = (uint32_t)stk;

    runq_push(t);
    return t;
}

uint32_t sched_on_tick(uint32_t current_saved_esp) {
    // If scheduler not enabled yet, keep running current
    if (g_enabled == 0) {
        return current_saved_esp;
    }

    // Make sure we have a "current task" to save into (idle)
    ensure_idle_task(current_saved_esp);

    // Save the current context into current task
    g_current->saved_esp = current_saved_esp;
    if (g_current->state != TASK_ZOMBIE) {
        g_current->state = TASK_READY;
        if (g_current->proc != NULL) {
            ((process_t *)g_current->proc)->state = PROC_READY;
        }
    } else if (g_current->proc != NULL) {
        ((process_t *)g_current->proc)->state = PROC_ZOMBIE;
    }

    if (g_left > 0) {
        g_left--;
    }
    if (g_left != 0 && g_current->state != TASK_ZOMBIE) {
        // Continue running current task
        g_current->state = TASK_RUNNING;
        process_on_tick(g_current);
        return g_current->saved_esp;
    }

    // Time slice expired: pick next runnable
    g_left = g_quantum;

    task_t *prev = g_current;
    task_t *next = runq_next_runnable(g_current);
    if (next == NULL) {
        // Nothing runnable -> run idle (current)
        g_current->state = TASK_RUNNING;
        return g_current->saved_esp;
    }

    g_current = next;
    g_current->state = TASK_RUNNING;
    process_on_tick(g_current);
    if (prev != NULL && prev->is_user == 0 && g_current->is_user != 0) {
        trace_log_return(g_current);
    }
    if (g_current->page_dir_phys != 0) {
        vmm_switch_pd(g_current->page_dir_phys);
    }
    if (g_current->kstack_top != 0) {
        tss_set_kernel_stack(g_current->kstack_top);
    }
    sched_reap_zombies();
    return g_current->saved_esp;
}

void sched_reap_zombies(void) {
    if (g_runq == NULL) {
        return;
    }
    int count = 0;
    task_t *t = g_runq;
    do {
        count++;
        t = t->next_task;
    } while (t != NULL && t != g_runq);

    t = g_runq;
    for (int i = 0; i < count && g_runq != NULL; i++) {
        task_t *next = t->next_task;
        if (t->state == TASK_ZOMBIE && t != g_current) {
            runq_remove(t);
            process_reap_task(t);
            if (t->kstack_base != NULL) {
                kfree(t->kstack_base);
            }
            if (t->stack_base != NULL && t->stack_base != t->kstack_base) {
                kfree(t->stack_base);
            }
            kfree(t);
        }
        t = next;
    }
}
