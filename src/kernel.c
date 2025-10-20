#include"vga.h"
#include"stdint.h"
#include "idt.h"
#include "util.h"

void kmain(void);

void kmain(void)
{
     reset();
    // print("Hello world\thello\n");
    //  for (;;) __asm__ __volatile__("hlt");

    init_idt();
    print("IDT Initialized\n");

   // uint8_t mask = InPortByte(0x21) & ~(1 << 1);
   // OutPortByte(0x21, mask);
    OutPortByte(0x21, InPortByte(0x21) & ~(1<<1));
    asm volatile("sti");

     for (;;)
        asm volatile("hlt");
}

