#ifndef SCHED_H
#define SCHED_H

#include "../utils/stdint.h"
#include "../utils/util.h"
#include "../memory/kheap.h"
#include "../utils/vga.h"

#define STACK_SIZE_DEFAULT 8192 // 8 KB


typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

typedef struct task_regs {
    uint32_t cr2;
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, useresp, ss;
} task_regs_t;

typedef struct task {
    uint32_t pid;
    task_state_t state;
    task_regs_t context;
    void* stack_base;
    uint32_t stack_size;
    struct task* next_task;
} task_t;

uint32_t init_task(task_t* task, uint32_t pid, void (*entry)(void));
#endif 
