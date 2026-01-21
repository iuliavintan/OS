#include "syscalls.h"
#include "utils/vga.h"
#include "timers/timer.h"
#include "scheduling/sched.h"
#include "process.h"

static uint32_t syscall_write(uint32_t fd, const char *buf, uint32_t len) {
    if (fd != 1 || buf == NULL) {
        return 0xFFFFFFFFu;
    }
    task_t *current = sched_current();
    if (current != NULL && current->is_user != 0) {
        process_t *proc = (process_t *)current->proc;
        uint16_t row = 0;
        int fixed = 0;
        if (proc != NULL && proc->name[0] == 'U' && proc->name[1] == '1') {
            row = UI_STATUS_ROW;
            fixed = 1;
        } else if (proc != NULL && proc->name[0] == 'U' && proc->name[1] == '2') {
            row = UI_STATUS_ROW + 1;
            fixed = 1;
        }
        if (fixed != 0) {
            uint16_t saved_x, saved_y;
            get_cursor_position(&saved_x, &saved_y);
            update_cursor(0, row);
            uint32_t col = 0;
            for (uint32_t i = 0; i < len && col < vga_width; i++) {
                char c = buf[i];
                if (c == '\r' || c == '\n') {
                    break;
                }
                putc(c);
                col++;
            }
            while (col < vga_width) {
                putc(' ');
                col++;
            }
            update_cursor(saved_x, saved_y);
            return len;
        }
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
            regs->eax = 0xFFFFFFFFu;
            return current_saved_esp;
    }
}
