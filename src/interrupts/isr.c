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


void common_isr_handler(struct IntrerruptRegisters* regs)
{
    if(regs->int_no<32)
    {
        print(exception_messages[regs->int_no]);
        print("\n");
        print("System Halted!");
        while(1);
    }
}

void page_fault_handler(uint32_t err_no, uint32_t cr2_fault_addr){
    uint32_t mask = 0x0000001;
    print("\n#PF at VA=%p  err=%08x\n", (void*)cr2_fault_addr, err_no);

    print("Cause: ");
    if( err_no & mask){
        print("The page fault was caused by a page-protection violation\n");
    }
    else{
        print("Non-present page\n");
    }
    print("Access: ");
    if( err_no & (mask << 1)){
        print("Write\n");
    }
    else{
        print("Read\n");
    }
    print("From: ");

    if( err_no & (mask << 2)){
        print("User\n");
    }
    else{
        print("Kernel\n");
    }

    print("ifetch= ");
    if( err_no & (mask << 3)){
        print("yes\n");
    }
    else {
        print("no\n");
    }

    print("reserved write= ");
    if(err_no & (mask << 4)){
        print("yes\n");
    }
    else{
        print("no\n");
    }

    print("protection_key= ");
    if(err_no & (mask << 4)){
        print("yes\n");
    }
    else{
        print("no\n");
    }
    
    print("shadow_stack= ");
    if(err_no & (mask << 4)){
        print("yes\n");
    }
    else{
        print("no\n");
    }

    print("Soft Guard Extensions= ");
    if(err_no & (mask << 4)){
        print("yes\n");
    }
    else{
        print("no\n");
    }
}

void isr_dispatch(struct IntrerruptRegisters* regs){
    (void)regs;
    uint32_t err_no = regs->int_no;
    switch(err_no){
        case 14: { 
           //uint32_t cr2 = read_cr2();
           uint32_t cr2 = 0;
           page_fault_handler(err_no, cr2);
            break;
        }
        default:
            common_isr_handler(regs);
            break;
    }
}
