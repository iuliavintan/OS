#include "utils/stdint.h"

#define VGA_BASE 0xB8000
#define VGA_COLS 80
#define VGA_ATTR 0x0F

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static uint16_t cur_row = 10;
static uint16_t cur_col = 0;

static void vga_putc(char c) {
    volatile uint16_t *vga = (uint16_t *)VGA_BASE;
    if (c == '\n') {
        cur_row++;
        cur_col = 0;
        return;
    }
    if (cur_col >= VGA_COLS) {
        cur_row++;
        cur_col = 0;
    }
    vga[cur_row * VGA_COLS + cur_col] = (uint16_t)(VGA_ATTR << 8) | (uint8_t)c;
    cur_col++;
}

static void vga_print(const char *s) {
    while (*s) {
        vga_putc(*s++);
    }
}

static void vga_print_int(int32_t v) {
    if (v == 0) {
        vga_putc('0');
        return;
    }
    if (v < 0) {
        vga_putc('-');
        v = -v;
    }
    char buf[12];
    int i = 0;
    while (v > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i > 0) {
        vga_putc(buf[--i]);
    }
}

static const char scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6',
    '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '[', ']', '\n', 0,  'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*',
    0,  ' ', 0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0
};

static char read_key(void) {
    for (;;) {
        if (inb(0x64) & 1) {
            uint8_t sc = inb(0x60);
            if (sc & 0x80) {
                continue;
            }
            return scancode_map[sc];
        }
    }
}

static int read_line(char *buf, int max) {
    int len = 0;
    for (;;) {
        char c = read_key();
        if (c == '\n') {
            vga_putc('\n');
            buf[len] = 0;
            return len;
        }
        if (c == '\b') {
            if (len > 0) {
                len--;
                if (cur_col > 0) {
                    cur_col--;
                } else if (cur_row > 0) {
                    cur_row--;
                    cur_col = VGA_COLS - 1;
                }
                vga_putc(' ');
                if (cur_col > 0) {
                    cur_col--;
                }
            }
            continue;
        }
        if (c == 0) {
            continue;
        }
        if (len < max - 1) {
            buf[len++] = c;
            vga_putc(c);
        }
    }
}

static int parse_int(const char **p) {
    int32_t sign = 1;
    int32_t val = 0;
    while (**p == ' ') (*p)++;
    if (**p == '-') {
        sign = -1;
        (*p)++;
    }
    while (**p >= '0' && **p <= '9') {
        val = val * 10 + (**p - '0');
        (*p)++;
    }
    return (int)(val * sign);
}

void calc_main(void) {
    uint8_t mask = inb(0x21);
    outb(0x21, mask | (1 << 1)); /* mask IRQ1 so keyboard IRQ doesn't print */

    vga_print("calc> ");
    char line[64];
    while (read_line(line, (int)sizeof(line)) > 0) {
        const char *p = line;
        int a = parse_int(&p);
        while (*p == ' ') p++;
        char op = *p++;
        int b = parse_int(&p);

        vga_print("= ");
        if (op == '+') {
            vga_print_int(a + b);
        } else if (op == '-') {
            vga_print_int(a - b);
        } else if (op == '*') {
            vga_print_int(a * b);
        } else if (op == '/') {
            if (b == 0) {
                vga_print("div0");
            } else {
                vga_print_int(a / b);
            }
        } else {
            vga_print("err");
        }
        vga_putc('\n');
        vga_print("calc> ");
    }
}
