#include "isr.h"


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
    print_error ("\n#PF at VA=%x  err=%08x\n", (void*)cr2_fault_addr, err_no);

    print_error("Cause: ");
    if( err_no & mask){
        print_error("The page fault was caused by a page-protection violation\n");
    }
    else{
        print_error("Non-present page\n");
    }
    print_error("Access: ");
    if( err_no & (mask << 1)){
        print_error("Write\n");
    }
    else{
        print_error("Read\n");
    }
    print_error("From: ");

    if( err_no & (mask << 2)){
        print_error("User\n");
    }
    else{
        print_error("Kernel\n");
    }

    print_error("ifetch= ");
    if( err_no & (mask << 3)){
        print_error("yes\n");
    }
    else {
        print_error("no\n");
    }

    print_error("reserved write= ");
    if(err_no & (mask << 4)){
        print_error("yes\n");
    }
    else{
        print_error("no\n");
    }

    print_error("protection_key= ");
    if(err_no & (mask << 4)){
        print_error("yes\n");
    }
    else{
        print_error("no\n");
    }
    
    print_error("shadow_stack= ");
    if(err_no & (mask << 4)){
        print_error("yes\n");
    }
    else{
        print_error("no\n");
    }

    print_error("Soft Guard Extensions= ");
    if(err_no & (mask << 4)){
        print_error("yes\n");
    }
    else{
        print_error("no\n");
    }
    print_error("PANIC: page fault, halting CPU\n");
    asm volatile("cli");
    for (;;)
        asm volatile("hlt");
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
