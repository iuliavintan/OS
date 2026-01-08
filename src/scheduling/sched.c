#include "sched.h"

static task_t *g_runq = NULL;        // circular list 
static task_t *g_current = NULL;     // currently running task
static uint32_t g_next_pid = 1;

static uint32_t g_quantum = 5;       // in timer ticks
static uint32_t g_left = 5;

static int g_enabled = 0;

static void runq_push(task_t *t) {
    if (!g_runq) {
        g_runq = t;
        t->next_task = t;
        return;
    }
    
    task_t *tail = g_runq;
    while (tail->next_task != g_runq) tail = tail->next_task;
    tail->next_task = t;
    t->next_task = g_runq;
}

static task_t* runq_next_runnable(task_t *from) {
    if (!from) return NULL;
    task_t *t = from->next_task;
    while (t != from) {
        if (t->state == TASK_READY || t->state == TASK_RUNNING) return t;
        t = t->next_task;
    }
    if (from->state == TASK_READY || from->state == TASK_RUNNING) return from;
    return NULL;
}

// We treat the kmain as an idle task the first time the scheduler runs.
// That way we can always switch back if needed.
static void ensure_idle_task(uint32_t current_saved_esp) {
    if (g_current) return;

    task_t *idle = (task_t*)kmalloc(sizeof(task_t));
    memset(idle, 0, sizeof(*idle));
    idle->pid = 0;
    idle->state = TASK_RUNNING;
    idle->saved_esp = current_saved_esp;
    idle->stack_base = NULL;
    idle->stack_size = 0;
    idle->next_task = NULL;

    g_current = idle;
    runq_push(idle);
}

void sched_init(uint32_t quantum_ticks) {
    g_quantum = (quantum_ticks == 0) ? 1 : quantum_ticks;
    g_left = g_quantum;
    g_enabled = 0;
    g_runq = NULL;
    g_current = NULL;
    g_next_pid = 1;
}

void sched_enable(int enabled) {
    g_enabled = enabled ? 1 : 0;
}

// Build an initial task_frame on the new task's stack so that when the stub restores
// regs + iretd, it lands at entry().
task_t* task_create(void (*entry)(void), uint32_t stack_size) {
    if (!stack_size) stack_size = STACK_SIZE_DEFAULT;

    task_t *t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) return NULL;
    memset(t, 0, sizeof(*t));

    t->pid = g_next_pid++;
    t->state = TASK_READY;
    t->stack_size = stack_size;
    t->stack_base = kmalloc(stack_size);
    if (!t->stack_base) {
        print_error("task_create: stack alloc failed\n");
        return NULL;
    }

    uint32_t top = (uint32_t)t->stack_base + t->stack_size;

    // Place task_frame_t at the top of the stack.
    task_frame_t *f = (task_frame_t*)(top - sizeof(task_frame_t));
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

uint32_t sched_on_tick(uint32_t current_saved_esp) {
    // If scheduler not enabled yet, keep running current
    if (!g_enabled) return current_saved_esp;

    // Make sure we have a "current task" to save into (idle)
    ensure_idle_task(current_saved_esp);

    // Save the current context into current task
    g_current->saved_esp = current_saved_esp;
    g_current->state = TASK_READY;

    if (g_left > 0) g_left--;
    if (g_left != 0) {
        // Continue running current task
        g_current->state = TASK_RUNNING;
        return g_current->saved_esp;
    }

    // Time slice expired: pick next runnable
    g_left = g_quantum;

    task_t *next = runq_next_runnable(g_current);
    if (!next) {
        // Nothing runnable -> run idle (current)
        g_current->state = TASK_RUNNING;
        return g_current->saved_esp;
    }

    g_current = next;
    g_current->state = TASK_RUNNING;
    return g_current->saved_esp;
}
