#include "shell.h"
#include "interrupts/irq.h"
#include "utils/util.h"
#include "utils/vga.h"
#include "memory/pmm.h"
#include "fs/fs.h"

#define SHELL_BUF_SIZE 64

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static void read_line(char *buf, int max) {
    int len = 0;
    for (;;) {
        int ch = kbd_getchar();
        if (ch < 0) {
            asm volatile("hlt");
            continue;
        }
        char c = (char)ch;
        if (c == '\r' || c == '\n') {
            putc('\n');
            buf[len] = 0;
            return;
        }
        if (c == '\b') {
            if (len > 0) {
                len--;
                putc('\b');
            }
            continue;
        }
        if (c < 32 || c > 126) {
            continue;
        }
        if (len < max - 1) {
            buf[len++] = c;
            putc(c);
        }
    }
}

static void cmd_help(void) {
    print("help  reboot  clear  ls  mem\n");
}

static void cmd_reboot(void) {
    OutPortByte(0x64, 0xFE);
    for (volatile uint32_t i = 0; i < 100000; i++) {
        asm volatile("" ::: "memory");
    }
    OutPortByte(0xCF9, 0x02);
    OutPortByte(0xCF9, 0x06);
    for (;;)
        asm volatile("hlt");
}

static void cmd_clear(void) {
    reset();
    update_cursor(0, 0);
}

struct ls_ctx {
    int count;
};

static void ls_cb(const char *name8, uint16_t start, void *ctx) {
    (void)start;
    struct ls_ctx *ls = (struct ls_ctx *)ctx;
    char name[9];
    int n = 0;
    for (int i = 0; i < 8; i++) {
        if (name8[i] == ' ') {
            break;
        }
        name[n++] = name8[i];
    }
    name[n] = 0;
    if (n > 0) {
        print("%s\n", name);
        ls->count++;
    }
}

static void cmd_ls(void) {
    if (!fs_ready()) {
        print("fs not initialized\n");
        return;
    }
    struct ls_ctx ctx = {0};
    fs_iterate_root(ls_cb, &ctx);
    if (ctx.count == 0) {
        print("(empty)\n");
    }
}

static void cmd_mem(void) {
    uint32_t total = (uint32_t)pmm_total_pages();
    uint32_t free = (uint32_t)pmm_available_pages();
    uint32_t total_mb = (total * PAGE_SIZE) / (1024 * 1024);
    uint32_t free_mb = (free * PAGE_SIZE) / (1024 * 1024);
    print("mem: total=%d pages (%d MB) free=%d pages (%d MB)\n",
          total, total_mb, free, free_mb);
}

void shell_run(void) {
    char line[SHELL_BUF_SIZE];
    for (;;) {
        print("> ");
        read_line(line, SHELL_BUF_SIZE);
        if (line[0] == 0) {
            continue;
        }
        if (streq(line, "help")) {
            cmd_help();
        } else if (streq(line, "reboot")) {
            cmd_reboot();
        } else if (streq(line, "clear")) {
            cmd_clear();
        } else if (streq(line, "ls")) {
            cmd_ls();
        } else if (streq(line, "mem")) {
            cmd_mem();
        } else {
            print("unknown command: %s\n", line);
        }
    }
}
