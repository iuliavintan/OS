#include "irq.h"
#include "../util.h"

void *irq_routines[16] = {              //irq routines associated with our interrupt requests
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

void irq_install_handler(int irq, void (*handler)(struct IntrerruptRegisters* r))
{
    irq_routines[irq] = handler;
}


void irq_uninstall_handler(int irq)
{
    irq_routines[irq]=0;
}

static uint8_t pic_isr_master(void){ OutPortByte(0x20, 0x0B); return InPortByte(0x20); }
static uint8_t pic_isr_slave (void){  OutPortByte(0xA0, 0x0B); return InPortByte(0xA0); }


void irq_handler(struct IntrerruptRegisters *regs)
{
    void (* handler)(struct IntrerruptRegisters *regs);

    int vec = regs->int_no;
    int irq_no = vec - 32;

    handler = irq_routines[irq_no];


    if(handler)
    {
        handler(regs);
    }

    if(vec >= 40)
    {
        OutPortByte(0xA0, 0x20);
    }
    else if(vec == 0x27){
         if (!(pic_isr_master() & (1 << 7))) {
            // Spurious: DO NOT send any EOI
            return;
        }
    }
    else if(vec == 0x2F){
         if (!(pic_isr_slave() & (1 << 7))) {
            // Spurious: DO NOT send any EOI
            OutPortByte(0x20, 0x20);
            return;
        }
    }

    OutPortByte(0x20, 0x20);

}

void irq0_handler(struct IntrerruptRegisters *r)
{
    (void)r;
    OutPortByte(0x20, 0x20);
    return;
}
static uint8_t kb_status(void) { return InPortByte(0x64); }
volatile uint32_t keyboard_irq_count = 0;
// 
// void irq1_handler(struct IntrerruptRegisters *r) {
    // (void)r;
    // while (kb_status() & 1) {
        // (void)InPortByte(0x60);  // read & drop scancode (store if you want)
    // }
    // keyboard_irq_count++;
// }


unsigned char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', 
    '7', '8', '9', '0', '-', '=', '\b', /* Backspace */
    '\t', /* Tab */
    'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
    0, /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    0, /* Right shift */
    '*',
    0, /* Alt */
    ' ', /* Space bar */
    0, /* Caps lock */
    /* restul tastei funcționale F1-F10 etc le poți lăsa 0 */
};

boolean caps_lock = 0;
boolean shift_pressed = 0;
void irq1_handler(struct IntrerruptRegisters *r) {
    (void)r;
    while (kb_status() & 1) {
        uint8_t sc = InPortByte(0x60);
        if (!(sc & 0x80)) {  // apăsare, nu eliberare
            if( sc == 0x3A ){
                caps_lock= ~caps_lock;
                char c = scancode_to_ascii[sc];
            }
            else if( )
            else{
                char c = scancode_to_ascii[sc]; 
                if(caps_lock) c = c - 32;   
                if (c)
                    print(&c);
            }
        }
    }
    keyboard_irq_count++;

    OutPortByte(0x20, 0x20);
}

void idt_enable_keyboard(void) {
    /* Register handler BEFORE unmasking */
    irq_install_handler(1, irq1_handler);

    /* Unmask only IRQ1 (keyboard) on master PIC; keep slave masked */
    OutPortByte(0x21, 0xFD);  // 1111 1101b -> enable IRQ1, others masked
    OutPortByte(0xA1, 0xFF);  // mask all on slave
}