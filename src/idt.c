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

    idt_flush((uint32_t)&idt_ptr);
}

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    // Implementation of setting an IDT gate
}