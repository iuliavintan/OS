#include "libu.h"

int sys_write(int fd, const char *buf, uint32_t len) {
    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(SYSCALL_WRITE), "b"(fd), "c"(buf), "d"(len)
                 : "memory");
    return ret;
}

void sys_yield(void) {
    asm volatile("int $0x80"
                 :
                 : "a"(SYSCALL_YIELD)
                 : "memory");
}

uint32_t sys_gettime(void) {
    uint32_t ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(SYSCALL_GETTIME)
                 : "memory");
    return ret;
}

uint32_t u_strlen(const char *s) {
    uint32_t n = 0;
    while (s != NULL && s[n] != 0) {
        n++;
    }
    return n;
}

void u_puts(const char *s) {
    sys_write(1, s, u_strlen(s));
}
