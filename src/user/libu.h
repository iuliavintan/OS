#ifndef LIBU_H
#define LIBU_H

#include "utils/stdint.h"

#define SYSCALL_WRITE   1
#define SYSCALL_YIELD   2
#define SYSCALL_GETTIME 3

int sys_write(int fd, const char *buf, uint32_t len);
void sys_yield(void);
uint32_t sys_gettime(void);
uint32_t u_strlen(const char *s);
void u_puts(const char *s);

#endif
