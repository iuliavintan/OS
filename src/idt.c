#include "stdint.h"
#include "util.h"
#include "vga.h"
#include "idt.h"

struct InterruptDescriptor32 idt[256];
struct idt_ptr_struct idt_ptr;

// extern void idt_flush(uint32_t);
extern void idt_flush(struct idt_ptr_struct*);

void init_idt() {
    idt_ptr.limit = (sizeof(struct InterruptDescriptor32) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt;

    memset(&idt, 0, sizeof(struct InterruptDescriptor32) * 256);

    // Here you would typically set up the IDT gates using set_idt_gate()

    OutPortByte(0x20,0x11); //start initialization of PIC
    OutPortByte(0xA0,0x11);

    OutPortByte(0x21,0x20); //remap offset of PIC
    OutPortByte(0xA1,0x28);

    OutPortByte(0x21,0x04); //setup cascading
    OutPortByte(0xA1,0x02);

    OutPortByte(0x21,0x01); //set 8086 mode
    OutPortByte(0xA1,0x01);

    OutPortByte(0x21,0xFF); //unmask all interrupts
    OutPortByte(0xA1,0xFF);

    //ar trebui puse astea pe 0xFF ca sa mascam toate interruptele
    


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

    set_idt_gate(32, (uint32_t)irq0 , 0x08, 0x8E);
    set_idt_gate(33, (uint32_t)irq1 , 0x08, 0x8E);
    set_idt_gate(34, (uint32_t)irq2 , 0x08, 0x8E);
    set_idt_gate(35, (uint32_t)irq3 , 0x08, 0x8E);
    set_idt_gate(36, (uint32_t)irq4 , 0x08, 0x8E);
    set_idt_gate(37, (uint32_t)irq5 , 0x08, 0x8E);
    set_idt_gate(38, (uint32_t)irq6 , 0x08, 0x8E); 
    set_idt_gate(39, (uint32_t)irq7 , 0x08, 0x8E);
    set_idt_gate(40, (uint32_t)irq8 , 0x08, 0x8E);
    set_idt_gate(41, (uint32_t)irq9 , 0x08, 0x8E);
    set_idt_gate(42, (uint32_t)irq10 , 0x08, 0x8E);
    set_idt_gate(43, (uint32_t)irq11 , 0x08, 0x8E);
    set_idt_gate(44, (uint32_t)irq12 , 0x08, 0x8E);
    set_idt_gate(45, (uint32_t)irq13 , 0x08, 0x8E);
    set_idt_gate(46, (uint32_t)irq14 , 0x08, 0x8E);
    set_idt_gate(47, (uint32_t)irq15 , 0x08, 0x8E);

    set_idt_gate(128 , (uint32_t)isr128 , 0x08, 0x8E); //System calls
    set_idt_gate(177, (uint32_t)isr177 , 0x08, 0x8E); // System calls


    // idt_flush((uint32_t)&idt_ptr);
    idt_flush(&idt_ptr);

    
    
}

void set_idt_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].sel = sel;
    idt[num].alwaysZero = 0;
    idt[num].flags = flags;
    idt[num].base_high = (base >> 16) & 0xFFFF;
}

 char * exception_messages[] = {
    "Devision By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Fault",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved", 
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void isr_handler(struct IntrerruptRegisters* regs)
{
    if(regs->int_no<32)
    {
        print(exception_messages[regs->int_no]);
        print("\n");
        print("System Halted!");
        while(1);
    }
}

void *irq_routines[16] = {              //irq routines associated with our interrupt requests
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

//  void (*irq_routines[16])(struct InterruptRegisters *r) = {0};

void irq_install_handler(int irq, void (*handler)(struct IntrerruptRegisters* r))
{
    irq_routines[irq] = handler;
}


void irq_uninstall_handler(int irq)
{
    irq_routines[irq]=0;
}

void irq_handler(struct IntrerruptRegisters *regs)
{
    void (* handler)(struct IntrerruptRegisters *regs);

    handler = irq_routines[regs->int_no - 32];

    if(handler)
    {
        handler(regs);
    }

    if(regs->int_no >= 40)
    {
        OutPortByte(0xA0, 0x20);
    }

    OutPortByte(0x20, 0x20);

}