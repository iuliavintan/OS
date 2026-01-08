#include "irq.h"
#include "../scheduling/sched.h"
#include "../process.h"
#include "../trace.h"
#include "../timers/timer.h"


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


uint32_t irq_handler(struct IntrerruptRegisters *regs)
{
    void (* handler)(struct IntrerruptRegisters *regs);

    int vec = regs->int_no;
    int irq_no = vec - 32;
    trace_log_entry("irq", regs, sched_current());

    uint32_t current_saved_esp = ((uint32_t)regs) + 4;
    uint32_t next_saved_esp = current_saved_esp;

    if(vec == 0x27){
        if (!(pic_isr_master() & (1 << 7))) {
            // Spurious: DO NOT send any EOI
            return current_saved_esp;
        }
    }
    
    if(vec == 0x2F){
        if (!(pic_isr_slave() & (1 << 7))) {
            // Spurious: DO NOT send any EOI
            OutPortByte(0x20, 0x20);
            return current_saved_esp;
        }
    }

    handler = irq_routines[irq_no];
    if(handler)
    {
        handler(regs);
    }

    if (irq_no == 0) {
        next_saved_esp = sched_on_tick(current_saved_esp);
    }

    if(vec >= 40)
    {
        OutPortByte(0xA0, 0x20);
    }
    else 

    OutPortByte(0x20, 0x20);
    return next_saved_esp;
}

void irq0_handler(struct IntrerruptRegisters *r)
{
    (void)r;
    OutPortByte(0x20, 0x20);
    return;
}
static uint8_t kb_status(void) { return InPortByte(0x64); }
volatile uint32_t keyboard_irq_count = 0;
 
#define KBD_BUF_SIZE 128
static volatile uint8_t kbd_buf[KBD_BUF_SIZE];
static volatile uint32_t kbd_head = 0;
static volatile uint32_t kbd_tail = 0;

static void kbd_push_char(uint8_t c) {
    uint32_t next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next == kbd_tail) {
        return;
    }
    kbd_buf[kbd_head] = c;
    kbd_head = next;
}

int kbd_getchar(void) {
    if (kbd_tail == kbd_head) {
        return -1;
    }
    uint8_t c = kbd_buf[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return (int)c;
}
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
boolean left_shift_pressed = 0;
boolean right_shift_pressed = 0;
static boolean ctrl_pressed = 0;
static uint8_t e0 = 0;
void irq1_handler(struct IntrerruptRegisters *r) {
    (void)r;
    while (kb_status() & 1) {
        uint8_t sc = InPortByte(0x60);
        if( sc == 0xe0) {
            e0 = 1;
            continue;
        }
        boolean key_released = sc & 0x80;
        uint8_t keycode = sc & 0x7F;

        if (e0) {
            e0 = 0;
            continue;
        }

        if (keycode == 0x2A) {
            left_shift_pressed = !key_released;
            continue;
        }
        if (keycode == 0x36) {
            right_shift_pressed = !key_released;
            continue;
        }
        if (keycode == 0x1D) {
            ctrl_pressed = !key_released;
            continue;
        }
        if (keycode == 0x3A) {
            if (!key_released) {
                caps_lock = !caps_lock;
            }
            continue;
        }
        if (key_released) {
            continue;
        }

        boolean shift_pressed = left_shift_pressed || right_shift_pressed;
        char c = scancode_to_ascii[keycode];
        if (!c) {
            continue;
        }

        if (c >= 'a' && c <= 'z') {
            if (shift_pressed ^ caps_lock) {
                c = (char)(c - 32);
            }
        } else if (shift_pressed) {
            switch (keycode) {
                case 2: c = '!'; break;
                case 3: c = '@'; break;
                case 4: c = '#'; break;
                case 5: c = '$'; break;
                case 6: c = '%'; break;
                case 7: c = '^'; break;
                case 8: c = '&'; break;
                case 9: c = '*'; break;
                case 10: c = '('; break;
                case 11: c = ')'; break;
                case 12: c = '_'; break;
                case 13: c = '+'; break;
                case 26: c = '{'; break;
                case 27: c = '}'; break;
                case 39: c = '"'; break;
                case 43: c = '|'; break;
                case 51: c = '<'; break;
                case 52: c = '>'; break;
                case 53: c = '?'; break;
                default: break;
            }
        }
        if (ctrl_pressed && (c == 't' || c == 'T')) {
            if (trace_get()) {
                trace_set(0);
                print("[TRACE] off\n");
            }
            if (tick_get()) {
                tick_set(0);
                print("[TICK] off\n");
            }
            continue;
        }
        if (ctrl_pressed && (c == 'c' || c == 'C')) {
            task_t *current = sched_current();
            if (current && current->is_user) {
                process_kill(current->pid);
            }
            continue;
        }
        kbd_push_char((uint8_t)c);
    }
    keyboard_irq_count++;
   // OutPortByte(0x20, 0x20);
}

void idt_enable_keyboard(void) {
    /* Register handler BEFORE unmasking */
    irq_install_handler(1, irq1_handler);

    /* Unmask only IRQ1 (keyboard) on master PIC; keep slave masked */
    OutPortByte(0x21, 0xFD);  // 1111 1101b -> enable IRQ1, others masked
    OutPortByte(0xA1, 0xFF);  // mask all on slave
}
