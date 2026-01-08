#include "syscalls.h"
#include "utils/vga.h"
#include "timers/timer.h"
#include "scheduling/sched.h"

static uint32_t syscall_write(uint32_t fd, const char *buf, uint32_t len) {
    if (fd != 1 || buf == NULL) {
        return (uint32_t)-1;
    }
    for (uint32_t i = 0; i < len; i++) {
        putc(buf[i]);
    }
    return len;
}

uint32_t syscall_dispatch(struct IntrerruptRegisters *regs, uint32_t current_saved_esp) {
    uint32_t num = regs->eax;
    switch (num) {
        case SYSCALL_WRITE:
            regs->eax = syscall_write(regs->ebx, (const char *)regs->ecx, regs->edx);
            return current_saved_esp;
        case SYSCALL_YIELD:
            regs->eax = 0;
            return sched_on_tick(current_saved_esp);
        case SYSCALL_GETTIME:
            regs->eax = (uint32_t)timer_get_ticks();
            return current_saved_esp;
        default:
            regs->eax = (uint32_t)-1;
            return current_saved_esp;
    }
}
