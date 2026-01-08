#include "trace.h"
#include "utils/vga.h"

static int g_trace_enabled = 0;

void trace_set(int enabled) {
    g_trace_enabled = enabled ? 1 : 0;
}

int trace_get(void) {
    return g_trace_enabled;
}

static uint32_t trace_get_cs(struct IntrerruptRegisters *regs) {
    uint32_t *p = (uint32_t *)((uintptr_t)regs + sizeof(*regs));
    return p[1];
}

void trace_log_entry(const char *kind, struct IntrerruptRegisters *regs, task_t *task) {
    if (!g_trace_enabled || !regs) {
        return;
    }
    uint32_t cs = trace_get_cs(regs);
    uint32_t cpl = cs & 3u;
    const char *dir = (cpl == 3) ? "u->k" : "k->k";
    uint32_t pid = task ? task->pid : 0;
    print("[TRACE] %s %s int=0x%x pid=%d cpl=%d\n", dir, kind, regs->int_no, pid, cpl);
}

void trace_log_return(task_t *next_task) {
    if (!g_trace_enabled || !next_task) {
        return;
    }
    if (!next_task->is_user) {
        return;
    }
    print("[TRACE] k->u pid=%d\n", next_task->pid);
}
