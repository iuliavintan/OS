#ifndef PROCESS_H
#define PROCESS_H

#include "utils/stdint.h"
#include "scheduling/sched.h"

typedef enum {
    PROC_READY = 0,
    PROC_RUNNING,
    PROC_ZOMBIE
} proc_state_t;

typedef struct process {
    uint32_t pid;
    proc_state_t state;
    uint32_t cpu_ticks;
    char name[9];
    task_t *task;
    struct process *next;
} process_t;

typedef struct {
    uint32_t pid;
    proc_state_t state;
    uint32_t cpu_ticks;
    char name[9];
} process_info_t;

void process_init(void);
process_t *process_exec(const char *name8);
void process_kill(uint32_t pid);
void process_list(void);
int process_snapshot(process_info_t *out, int max);
void process_on_tick(task_t *current);
void process_reap_task(task_t *t);

#endif
