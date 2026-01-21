#include "libu.h"

void _start(void) {
    for (;;) {
        uint32_t t = sys_gettime();
        char buf[12];
        int i = 0;
        const char *prefix = "u1: tick=";
        for (int p = 0; prefix[p] != 0; p++) {
            buf[i++] = prefix[p];
        }
        if (t == 0) {
            buf[i++] = '0';
        } else {
            char tmp[12];
            int j = 0;
            int tmp_limit = sizeof(tmp);
            while (t > 0 && j < tmp_limit) {
                tmp[j++] = '0' + (t % 10);
                t /= 10;
            }
            while (j > 0) {
                buf[i++] = tmp[--j];
            }
        }
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = ' ';
        buf[i++] = '\r';
        {
            uint32_t len = i;
            sys_write(1, buf, len);
        }
        sys_yield();
    }
}
