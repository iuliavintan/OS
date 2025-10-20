#include "stdint.h"
#include "util.h"
#include "vga.h"
#include "idt.h"

struct InterruptDescriptor32 idt[256];
struct idt_ptr_struct idt_ptr;

extern void idt_flush(uint32_t);

void init_idt() {
    idt_ptr.limit = (sizeof(struct InterruptDescriptor32) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;

    memset(&idt, 0, sizeof(struct InterruptDescriptor32) * 256);

    // Here you would typically set up the IDT gates using set_idt_gate()

    outPortB(0x20,0x11); //start initialization of PIC
    outPortB(0xA0,0x11);

    outPortB(0x21,0x20); //remap offset of PIC
    outPortB(0xA1,0x28);

    outPortB(0x21,0x04); //setup cascading
    outPortB(0xA1,0x02);

    outPortB(0x21,0x01); //set 8086 mode
    outPortB(0xA1,0x01);

    outPortB(0x21,0x0); //unmask all interrupts
    outPortB(0xA1,0x0);


    set_idt_gate(0 , (uint32_t)isr0 , 0x08, 0x8E);
    set_idt_gate(1 , (uint32_t)isr1 , 0x08, 0x8E);
    set_idt_gate(2 , (uint32_t)isr2 , 0x08, 0x8E);
    set_idt_gate(3 , (uint32_t)isr3 , 0x08, 0x8E);
    set_idt_gate(4 , (uint32_t)isr4 , 0x08, 0x8E);
    set_idt_gate(5 , (uint32_t)isr5 , 0x08, 0x8E);
    set_idt_gate(6 , (uint32_t)isr6 , 0x08, 0x8E);
    set_idt_gate(7 , (uint32_t)isr7 , 0x08, 0x8E);
    set_idt_gate(8 , (uint32_t)isr8 , 0x08, 0x8E);
    set_idt_gate(9 , (uint32_t)isr9 , 0x08, 0x8E);
    set_idt_gate(10 , (uint32_t)isr10 , 0x08, 0x8E);
    set_idt_gate(11 , (uint32_t)isr11 , 0x08, 0x8E);
    set_idt_gate(12 , (uint32_t)isr12 , 0x08, 0x8E);
    set_idt_gate(13 , (uint32_t)isr13 , 0x08, 0x8E);
    set_idt_gate(14 , (uint32_t)isr14 , 0x08, 0x8E);
    set_idt_gate(15 , (uint32_t)isr15 , 0x08, 0x8E);
    set_idt_gate(16 , (uint32_t)isr16 , 0x08, 0x8E);
    set_idt_gate(17 , (uint32_t)isr17 , 0x08, 0x8E);
    set_idt_gate(18 , (uint32_t)isr18 , 0x08, 0x8E);
    set_idt_gate(19 , (uint32_t)isr19 , 0x08, 0x8E);
    set_idt_gate(20 , (uint32_t)isr20 , 0x08, 0x8E);
    set_idt_gate(21 , (uint32_t)isr21 , 0x08, 0x8E);
    set_idt_gate(22 , (uint32_t)isr22 , 0x08, 0x8E);
    set_idt_gate(23 , (uint32_t)isr23 , 0x08, 0x8E);
    set_idt_gate(24 , (uint32_t)isr24 , 0x08, 0x8E);
    set_idt_gate(25 , (uint32_t)isr25 , 0x08, 0x8E);
    set_idt_gate(26 , (uint32_t)isr26 , 0x08, 0x8E);
    set_idt_gate(27 , (uint32_t)isr27 , 0x08, 0x8E);
    set_idt_gate(28 , (uint32_t)isr28 , 0x08, 0x8E);
    set_idt_gate(29 , (uint32_t)isr29 , 0x08, 0x8E);
    set_idt_gate(30 , (uint32_t)isr30 , 0x08, 0x8E);
    set_idt_gate(31 , (uint32_t)isr31 , 0x08, 0x8E);

    set_idt_gate(128 , (uint32_t)isr128 , 0x08, 0x8E); //System calls
    set_idt_gate(177, (uint32_t)isr177 , 0x08, 0x8E); // System calls


    idt_flush((uint32_t)&idt_ptr);
}

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].sel = sel;
    idt[num].alwaysZero = 0;
    idt[num].flags = flags | 0x60;
    idt[num].base_high = (base >> 16) & 0xFFFF;
}