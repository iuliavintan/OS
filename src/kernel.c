#include"vga.h"

//void kmain(void);

void kmain(void)
{
    reset();
    print("Hello world\r\n");
    for (;;) __asm__ __volatile__("hlt");
}

