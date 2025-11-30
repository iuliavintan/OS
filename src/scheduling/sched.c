#include "sched.h"

void init_task(task_t* task, uint32_t pid) {
    task->pid = pid;
    task->state = 0; 
    task->stack_size = STACK_SIZE_DEFAULT;
    task->stack_base = kmalloc(STACK_SIZE_DEFAULT);
    if(task->stack_base == NULL) {
        print_error("Failed to allocate memory for task stack");
        return;
    }
    task->context.esp = (uint32_t)task->stack_base + STACK_SIZE_DEFAULT;
    task->context.ebp = (uint32_t)task->stack_base;
    task->next_task = NULL;
}