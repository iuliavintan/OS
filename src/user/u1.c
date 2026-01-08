#include "libu.h"

void _start(void) {
    for (;;) {
        u_puts("u1: tick=");
        uint32_t t = sys_gettime();
        char buf[12];
        int i = 0;
        if (t == 0) {
            buf[i++] = '0';
        } else {
            char tmp[12];
            int j = 0;
            while (t > 0 && j < (int)sizeof(tmp)) {
                tmp[j++] = (char)('0' + (t % 10));
                t /= 10;
            }
            while (j > 0) {
                buf[i++] = tmp[--j];
            }
        }
        buf[i++] = '\n';
        sys_write(1, buf, (uint32_t)i);
        sys_yield();
    }
}
