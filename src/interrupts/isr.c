#include "isr.h"
#include "../util.h"
#include "../vga.h"

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