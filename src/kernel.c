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
  //  set();
    kprint("[KERNEL] Booting...\n");

    init_idt();
    //print("IDT Initialized\n");

    idt_enable_keyboard();
    initTimer();

    uint8_t mask = InPortByte(0x21);
    mask &= ~(1 << 0); // dezactivează masca pentru IRQ0 (timer)
    OutPortByte(0x21, mask);
   // print("[KERNEL] Booting...");
    asm volatile("sti");

     for (;;)
        asm volatile("hlt");
}

