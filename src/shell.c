#include "shell.h"
#include "interrupts/irq.h"
#include "utils/util.h"
#include "utils/vga.h"
#include "memory/pmm.h"
#include "memory/kheap.h"
#include "timers/timer.h"
#include "fs/fs.h"
#include "process.h"
#include "trace.h"

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
    print("help  reboot  clear  ls  mem  heap  heapdemo  tick  ps  kill  exec  trace\n");
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
    shell_print_banner();
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
        if (streq(name, "STAGE2") || streq(name, "KERNEL")) {
            return;
        }
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

static void cmd_heap(void) {
    heap_dump();
}

static void cmd_heapdemo(const char *arg) {
    static void *a;
    static void *b;
    static void *c;
    static void *d;
    if (arg && streq(arg, "free")) {
        if (a) { kfree(a); a = NULL; }
        if (b) { kfree(b); b = NULL; }
        if (c) { kfree(c); c = NULL; }
        if (d) { kfree(d); d = NULL; }
        print("heapdemo: freed\n");
        return;
    }
    if (a || b || c || d) {
        print("heapdemo: already allocated, use `heapdemo free`\n");
        return;
    }
    a = kmalloc(32);
    b = kmalloc(64);
    c = kmalloc(128);
    kfree(b);
    b = NULL;
    d = kmalloc(40);
    print("heapdemo: alloc a=%x c=%x d=%x (hole between a and c)\n", a, c, d);
}

static void cmd_tick(const char *arg) {
    if (!arg || *arg == 0) {
        print("tick is %s\n", tick_get() ? "on" : "off");
        return;
    }
    if (streq(arg, "on")) {
        tick_set(1);
        print("tick on\n");
        return;
    }
    if (streq(arg, "off")) {
        tick_set(0);
        print("tick off\n");
        return;
    }
    print("usage: tick [on|off]\n");
}

static void cmd_ps(void) {
    process_list();
}

static void cmd_kill(const char *arg) {
    if (!arg || *arg == 0) {
        print("usage: kill <pid>\n");
        return;
    }
    uint32_t pid = 0;
    while (*arg >= '0' && *arg <= '9') {
        pid = pid * 10 + (uint32_t)(*arg - '0');
        arg++;
    }
    if (pid == 0) {
        print("kill: invalid pid\n");
        return;
    }
    process_kill(pid);
}

static void format_name8(const char *in, char out[9]) {
    for (int i = 0; i < 8; i++) {
        out[i] = ' ';
    }
    out[8] = 0;
    int i = 0;
    while (*in && i < 8) {
        char c = *in++;
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 32);
        }
        out[i++] = c;
    }
}

static void cmd_exec(const char *arg) {
    if (!arg || *arg == 0) {
        print("usage: exec <prog>\n");
        return;
    }
    char name8[9];
    format_name8(arg, name8);
    if (!process_exec(name8)) {
        print("exec failed\n");
    }
}

void shell_print_banner(void) {
    print("       _                        ___  ____  \n");
    print("__   _(_)_ __ ___   ___  _ __  / _ \\/ ___| \n");
    print("\\ \\ / / | '_ ` _ \\ / _ \\| '_ \\| | | \\___ \\ \n");
    print(" \\ V /| | | | | | | (_) | | | | |_| |___) |\n");
    print("  \\_/ |_|_| |_| |_|\\___/|_| |_|\\___/|____/ \n");
    print("\n");
    print("Welcome to vimonOS\n\n");
}

static void cmd_trace(const char *arg) {
    if (!arg || *arg == 0) {
        print("trace is %s\n", trace_get() ? "on" : "off");
        return;
    }
    if (streq(arg, "on")) {
        trace_set(1);
        print("trace on\n");
        return;
    }
    if (streq(arg, "off")) {
        trace_set(0);
        print("trace off\n");
        return;
    }
    print("usage: trace [on|off]\n");
}

void shell_run(void) {
    char line[SHELL_BUF_SIZE];
    for (;;) {
        print("> ");
        read_line(line, SHELL_BUF_SIZE);
        char *p = line;
        while (*p == ' ') p++;
        if (*p == 0) {
            continue;
        }
        char *cmd = p;
        while (*p && *p != ' ') p++;
        if (*p) {
            *p++ = 0;
        }
        while (*p == ' ') p++;
        char *arg = p;

        if (streq(cmd, "help")) {
            cmd_help();
        } else if (streq(cmd, "reboot")) {
            cmd_reboot();
        } else if (streq(cmd, "clear")) {
            cmd_clear();
        } else if (streq(cmd, "ls")) {
            cmd_ls();
        } else if (streq(cmd, "mem")) {
            cmd_mem();
        } else if (streq(cmd, "heap")) {
            cmd_heap();
        } else if (streq(cmd, "heapdemo")) {
            cmd_heapdemo(arg);
        } else if (streq(cmd, "tick")) {
            cmd_tick(arg);
        } else if (streq(cmd, "ps")) {
            cmd_ps();
        } else if (streq(cmd, "kill")) {
            cmd_kill(arg);
        } else if (streq(cmd, "exec")) {
            cmd_exec(arg);
        } else if (streq(cmd, "trace")) {
            cmd_trace(arg);
        } else {
            print("unknown command: %s\n", cmd);
        }
    }
}
