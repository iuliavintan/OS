#include "libu.h"

void _start(void) {
    for (;;) {
        u_puts("u2: hello from user mode        \r");
        sys_yield();
    }
}
