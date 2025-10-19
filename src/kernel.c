#include"vga.h"
#include"stdint.h"


void kmain(void);

void kmain(void)
{
    reset();
    print("Hello world\n");
   // for (;;) __asm__ __volatile__("hlt");
}

