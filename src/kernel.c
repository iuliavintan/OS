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
    //disable_cursor();
    kprint("[KERNEL] Booting...\n");
    init_idt();
    idt_enable_keyboard();
    initTimer();
    kprint("[KERNEL] PM OK, IDT OK, IRQ0 OK, IRQ1 OK\n");
    uint8_t mask = InPortByte(0x21);
    mask &= ~(1 << 0); // dezactivează masca pentru IRQ0 (timer)
    OutPortByte(0x21, mask);
   // print("[KERNEL] Booting...");
    asm volatile("sti");

     for (;;)
        asm volatile("hlt");
}

