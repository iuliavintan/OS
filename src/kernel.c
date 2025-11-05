#include"vga.h"
#include"stdint.h"
#include "interrupts/idt.h"
#include "interrupts/irq.h"
#include "interrupts/isr.h"
#include "util.h"
#include"timers/timer.h"
#include "memory/e820.h"
#include "memory/pmm.h"

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
    e820_import();
    uint64_t total_ram = e820_get_usable_ram();
    print("Total usable RAM: %d MB\n", (uint32_t)(total_ram / (1024*1024)));

    pmm_init();

    uint8_t mask = InPortByte(0x21);
    mask &= ~(1 << 0); // dezactivează masca pentru IRQ0 (timer)
    OutPortByte(0x21, mask);
    asm volatile("sti");

     for (;;)
        asm volatile("hlt");
}

