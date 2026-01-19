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
#define HISTORY_MAX 32
#define TOP_MAX_PROCS 32

static const char *builtin_cmds[] = {
    "help", "reboot", "clear", "ls", "mem", "heap", "heapdemo",
    "ps", "kill", "exec", "trace", "top", 0
};

static char history[HISTORY_MAX][SHELL_BUF_SIZE];
static int history_len = 0;

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

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

static void history_add(const char *line) {
    if (!line || *line == 0) {
        return;
    }
    if (history_len > 0 && streq(history[history_len - 1], line)) {
        return;
    }
    if (history_len < HISTORY_MAX) {
        int i = 0;
        while (line[i] && i < SHELL_BUF_SIZE - 1) {
            history[history_len][i] = line[i];
            i++;
        }
        history[history_len][i] = 0;
        history_len++;
        return;
    }
    for (int i = 1; i < HISTORY_MAX; i++) {
        int j = 0;
        while (history[i][j]) {
            history[i - 1][j] = history[i][j];
            j++;
        }
        history[i - 1][j] = 0;
    }
    int k = 0;
    while (line[k] && k < SHELL_BUF_SIZE - 1) {
        history[HISTORY_MAX - 1][k] = line[k];
        k++;
    }
    history[HISTORY_MAX - 1][k] = 0;
}

static void shell_print_prompt(void) {
    vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
    print("vimonOS");
    vga_reset_color();
    print(" > ");
}

static const char *completion_first(const char *prefix) {
    if (!prefix || *prefix == 0) {
        return 0;
    }
    for (int i = history_len - 1; i >= 0; i--) {
        if (starts_with(history[i], prefix)) {
            return history[i];
        }
    }
    for (int i = 0; builtin_cmds[i]; i++) {
        if (starts_with(builtin_cmds[i], prefix)) {
            return builtin_cmds[i];
        }
    }
    return 0;
}

static const char *state_str(proc_state_t state) {
    if (state == PROC_RUNNING) return "RUN";
    if (state == PROC_ZOMBIE) return "ZOMB";
    return "READY";
}

static int find_prev_ticks(process_info_t *prev, int prev_count, uint32_t pid, uint32_t *out_ticks) {
    for (int i = 0; i < prev_count; i++) {
        if (prev[i].pid == pid) {
            *out_ticks = prev[i].cpu_ticks;
            return 1;
        }
    }
    return 0;
}

static const char *completion_next(const char *prefix, const char *current) {
    const char *first = 0;
    int passed = current == 0;
    if (!prefix || *prefix == 0) {
        return 0;
    }
    for (int i = history_len - 1; i >= 0; i--) {
        if (!starts_with(history[i], prefix)) {
            continue;
        }
        if (!first) {
            first = history[i];
        }
        if (passed) {
            return history[i];
        }
        if (current && streq(history[i], current)) {
            passed = 1;
        }
    }
    for (int i = 0; builtin_cmds[i]; i++) {
        if (!starts_with(builtin_cmds[i], prefix)) {
            continue;
        }
        if (!first) {
            first = builtin_cmds[i];
        }
        if (passed) {
            return builtin_cmds[i];
        }
        if (current && streq(builtin_cmds[i], current)) {
            passed = 1;
        }
    }
    if (current && first) {
        return first;
    }
    return 0;
}

static void line_replace(char *buf, int *len, const char *text) {
    while (*len > 0) {
        putc('\b');
        (*len)--;
    }
    int i = 0;
    while (text[i] && i < SHELL_BUF_SIZE - 1) {
        buf[i] = text[i];
        putc(text[i]);
        i++;
    }
    buf[i] = 0;
    *len = i;
}

static void clear_ghost(int last_len) {
    if (last_len <= 0) {
        return;
    }
    for (int i = 0; i < last_len; i++) {
        putc(' ');
    }
    uint16_t x, y;
    get_cursor_position(&x, &y);
    if (x >= (uint16_t)last_len) {
        update_cursor(x - (uint16_t)last_len, y);
    }
}

static int render_ghost(const char *buf, int len) {
    const char *match = completion_first(buf);
    if (!match) {
        return 0;
    }
    int i = len;
    int ghost_len = 0;
    if (match[i] == 0) {
        return 0;
    }
    vga_set_color(COLOUR8_DARK_GREY, COLOUR8_BLACK);
    while (match[i]) {
        putc(match[i]);
        ghost_len++;
        i++;
    }
    vga_reset_color();
    uint16_t x, y;
    get_cursor_position(&x, &y);
    if (x >= (uint16_t)ghost_len) {
        update_cursor(x - (uint16_t)ghost_len, y);
    }
    return ghost_len;
}

static void read_line(char *buf, int max) {
    int len = 0;
    int hist_index = history_len;
    int browsing = 0;
    char saved[SHELL_BUF_SIZE];
    char completion_prefix[SHELL_BUF_SIZE];
    const char *completion_current = 0;
    completion_prefix[0] = 0;
    int last_ghost = 0;
    for (;;) {
        int key = kbd_getchar();
        if (key < 0) {
            asm volatile("hlt");
            continue;
        }
        if (key == '\r' || key == '\n') {
            clear_ghost(last_ghost);
            last_ghost = 0;
            putc('\n');
            buf[len] = 0;
            return;
        }
        if (key == '\b') {
            clear_ghost(last_ghost);
            last_ghost = 0;
            if (len > 0) {
                len--;
                putc('\b');
                buf[len] = 0;
            }
            browsing = 0;
            hist_index = history_len;
            completion_current = 0;
            completion_prefix[0] = 0;
            last_ghost = render_ghost(buf, len);
            continue;
        }
        if (key == KBD_KEY_UP || key == KBD_KEY_DOWN) {
            clear_ghost(last_ghost);
            last_ghost = 0;
            if (!browsing) {
                int i = 0;
                while (buf[i] && i < SHELL_BUF_SIZE - 1) {
                    saved[i] = buf[i];
                    i++;
                }
                saved[i] = 0;
                browsing = 1;
            }
            if (history_len == 0) {
                continue;
            }
            if (key == KBD_KEY_UP) {
                if (hist_index > 0) {
                    hist_index--;
                    line_replace(buf, &len, history[hist_index]);
                }
            } else {
                if (hist_index < history_len - 1) {
                    hist_index++;
                    line_replace(buf, &len, history[hist_index]);
                } else {
                    hist_index = history_len;
                    line_replace(buf, &len, saved);
                    browsing = 0;
                }
            }
            completion_current = 0;
            completion_prefix[0] = 0;
            last_ghost = render_ghost(buf, len);
            continue;
        }
        if (key == KBD_KEY_RIGHT) {
            clear_ghost(last_ghost);
            last_ghost = 0;
            completion_current = 0;
            completion_prefix[0] = 0;
            const char *match = completion_first(buf);
            if (match) {
                line_replace(buf, &len, match);
            }
            browsing = 0;
            hist_index = history_len;
            last_ghost = render_ghost(buf, len);
            continue;
        }
        if (key == '\t') {
            clear_ghost(last_ghost);
            last_ghost = 0;
            if (completion_prefix[0] == 0) {
                int i = 0;
                while (buf[i] && i < SHELL_BUF_SIZE - 1) {
                    completion_prefix[i] = buf[i];
                    i++;
                }
                completion_prefix[i] = 0;
            }
            completion_current = completion_next(completion_prefix, completion_current);
            if (completion_current) {
                line_replace(buf, &len, completion_current);
            }
            browsing = 0;
            hist_index = history_len;
            last_ghost = render_ghost(buf, len);
            continue;
        }
        if (key == KBD_KEY_LEFT) {
            continue;
        }
        if (key < 32 || key > 126) {
            continue;
        }
        if (len < max - 1) {
            clear_ghost(last_ghost);
            last_ghost = 0;
            buf[len++] = (char)key;
            buf[len] = 0;
            putc((char)key);
        }
        browsing = 0;
        hist_index = history_len;
        completion_current = 0;
        completion_prefix[0] = 0;
        last_ghost = render_ghost(buf, len);
    }
}

static void cmd_help(void) {
    print("help  reboot  clear  ls  mem  heap  heapdemo  ps  kill  exec  trace  top\n");
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

static void cmd_ps(void) {
    process_list();
}

static void cmd_top(void) {
    process_info_t cur[TOP_MAX_PROCS];
    process_info_t prev[TOP_MAX_PROCS];
    uint16_t screen[vga_width * vga_height];
    volatile uint16_t *vga = (uint16_t *)0xB8000;
    uint16_t saved_x, saved_y;
    get_cursor_position(&saved_x, &saved_y);
    for (int i = 0; i < vga_width * vga_height; i++) {
        screen[i] = vga[i];
    }
    int prev_count = 0;
    uint64_t last_tick = timer_get_ticks();
    for (;;) {
        int key = kbd_getchar();
        if (key >= 0 && (key == 'q' || key == 'Q')) {
            break;
        }
        if (key < 0) {
            asm volatile("hlt");
        }
        uint64_t now = timer_get_ticks();
        if (now - last_tick < 25) {
            continue;
        }
        last_tick = now;
        int count = process_snapshot(cur, TOP_MAX_PROCS);
        reset();
        update_cursor(0, 0);
        vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
        print("top  ");
        vga_reset_color();
        print("ticks=%d  procs=%d  mem free=%d MB\n",
              (uint32_t)now,
              count,
              (uint32_t)((pmm_available_pages() * PAGE_SIZE) / (1024 * 1024)));
        print("PID  STATE  CPU  dCPU  NAME\n");
        for (int i = 0; i < count; i++) {
            uint32_t prev_ticks = 0;
            uint32_t delta = 0;
            if (find_prev_ticks(prev, prev_count, cur[i].pid, &prev_ticks)) {
                delta = cur[i].cpu_ticks - prev_ticks;
            }
            print("%d  %s  %d  %d  %s\n",
                  cur[i].pid,
                  state_str(cur[i].state),
                  cur[i].cpu_ticks,
                  delta,
                  cur[i].name);
        }
        for (int i = 0; i < count; i++) {
            prev[i] = cur[i];
        }
        prev_count = count;
        print("\nPress q to exit\n");
    }
    for (int i = 0; i < vga_width * vga_height; i++) {
        vga[i] = screen[i];
    }
    update_cursor(saved_x, saved_y);
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
    vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
    print("       _                        ");
    vga_set_color(COLOUR8_LIGHT_MANGENTA, COLOUR8_BLACK);
    print("___  ____  \n");
    vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
    print("__   _(_)_ __ ___   ___  _ __  ");
    vga_set_color(COLOUR8_LIGHT_MANGENTA, COLOUR8_BLACK);
    print("/ _ \\/ ___| \n");
    vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
    print("\\ \\ / / | '_ ` _ \\ / _ \\| '_ \\");
    vga_set_color(COLOUR8_LIGHT_MANGENTA, COLOUR8_BLACK);
    print("| | | \\___ \\ \n");
    vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
    print(" \\ V /| | | | | | | (_) | | | ");
    vga_set_color(COLOUR8_LIGHT_MANGENTA, COLOUR8_BLACK);
    print("| |_| |___) |\n");
    vga_set_color(COLOUR8_LIGHT_CYAN, COLOUR8_BLACK);
    print("  \\_/ |_|_| |_| |_|\\___/|_| |_|");
    vga_set_color(COLOUR8_LIGHT_MANGENTA, COLOUR8_BLACK);
    print("\\___/|____/ \n");
    vga_reset_color();
    print("\n");
    print("Welcome to vimonOS\n");
    vga_set_color(COLOUR8_DARK_GREY, COLOUR8_BLACK);
    print("Type `help` to list commands.\n");
    vga_reset_color();
    print("\n");
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
    char original[SHELL_BUF_SIZE];
    for (;;) {
        shell_print_prompt();
        read_line(line, SHELL_BUF_SIZE);
        int oi = 0;
        while (line[oi] && oi < SHELL_BUF_SIZE - 1) {
            original[oi] = line[oi];
            oi++;
        }
        original[oi] = 0;
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
        history_add(original);

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
        } else if (streq(cmd, "ps")) {
            cmd_ps();
        } else if (streq(cmd, "kill")) {
            cmd_kill(arg);
        } else if (streq(cmd, "top")) {
            cmd_top();
        } else if (streq(cmd, "exec")) {
            cmd_exec(arg);
        } else if (streq(cmd, "trace")) {
            cmd_trace(arg);
        } else {
            print("unknown command: %s\n", cmd);
        }
    }
}
