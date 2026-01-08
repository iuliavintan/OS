#ifndef TRACE_H
#define TRACE_H

#include "utils/stdint.h"
#include "utils/util.h"
#include "scheduling/sched.h"

void trace_set(int enabled);
int trace_get(void);
void trace_log_entry(const char *kind, struct IntrerruptRegisters *regs, task_t *task);
void trace_log_return(task_t *next_task);

#endif
