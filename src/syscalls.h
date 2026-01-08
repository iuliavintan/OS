#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "utils/stdint.h"
#include "utils/util.h"

#define SYSCALL_WRITE   1
#define SYSCALL_YIELD   2
#define SYSCALL_GETTIME 3

uint32_t syscall_dispatch(struct IntrerruptRegisters *regs, uint32_t current_saved_esp);

#endif
