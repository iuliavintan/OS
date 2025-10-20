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

    idt_flush((uint32_t)&idt_ptr);
}

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    // Implementation of setting an IDT gate
}