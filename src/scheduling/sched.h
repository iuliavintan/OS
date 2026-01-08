#ifndef SCHED_H
#define SCHED_H

#include "../utils/stdint.h"
#include "../utils/util.h"
#include "../memory/kheap.h"
#include "../utils/vga.h"

#define STACK_SIZE_DEFAULT 8192 // 8 KB

#define KERNEL_CS 0x08  //code=0x08
#define KERNEL_DS 0x10  //data=0x10

typedef enum {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

typedef struct task_frame {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} __attribute__((packed)) task_frame_t;

typedef struct task {
    uint32_t pid;
    task_state_t state;

    // Saved stack pointer that points to the start of task_frame_t (the gs field).
    uint32_t saved_esp;

    void *stack_base;
    uint32_t stack_size;
    void *kstack_base;
    uint32_t kstack_size;
    uint32_t kstack_top;
    uintptr_t page_dir_phys;
    int is_user;
    void *proc;

    struct task *next_task;
} task_t;


void     sched_init(uint32_t quantum_ticks);
void     sched_enable(int enabled);
task_t  *sched_current(void);
void     sched_set_foreground_pid(uint32_t pid);
uint32_t sched_get_foreground_pid(void);

task_t*   task_create(void (*entry)(void), uint32_t stack_size);
task_t*   task_create_user(uint32_t entry, uint32_t user_stack_top);

uint32_t  sched_on_tick(uint32_t current_saved_esp);
void      sched_reap_zombies(void);

#endif 
