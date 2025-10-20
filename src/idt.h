#ifndef IDT_H
#define IDT_H
#include "stdint.h"

struct InterruptDescriptor32 {
   uint16_t base_low;        // offset bits 0..15
   uint16_t sel;        // a code segment selector in GDT or LDT
   uint8_t  alwaysZero;            // unused, set to 0
   uint8_t  flags; // gate type, dpl, and p fields
   uint16_t base_high;        // offset bits 16..31
}__attribute__((packed));

struct idt_ptr_struct{
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

void init_idt();
void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif 