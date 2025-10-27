#include"vga.h"
#include"stdint.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "interrupts/isr.h"
#include "util.h"
#include"timers/timer.h"

void kmain(void);

void kmain(void)
{
     reset();
    // print("Hello world\thello\n");

    init_idt();
    print("IDT Initialized\n");

//    uint8_t mask = InPortByte(0x21) & ~(1 << 1);
//    OutPortByte(0x21, mask);
    //OutPortByte(0x21, InPortByte(0x21) & ~(1<<1));
    // Somewhere after init_idt() has set gates and before sti:
   // irq_install_handler(1, keyboard_handler);
    idt_enable_keyboard();
    initTimer();

    uint8_t mask = InPortByte(0x21);
    mask &= ~(1 << 0); // dezactivează masca pentru IRQ0 (timer)
    OutPortByte(0x21, mask);

    asm volatile("sti");

     for (;;)
        asm volatile("hlt");
}

