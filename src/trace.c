#include "trace.h"
#include "fs/fs.h"
#include "utils/util.h"
#include "syscalls.h"

static int g_trace_enabled = 0;
static int g_trace_max = 0;
static int g_trace_len = 0;
static char g_trace_buf[2048];
static const char g_trace_name8[9] = {'T','R','A','C','E',' ',' ',' ','\0'};

static int append_str(char *buf, int pos, int max, const char *s) {
    if (buf == NULL || s == NULL || max <= 0) {
        return pos;
    }
    while (*s != 0 && pos < max - 1) {
        buf[pos++] = *s++;
    }
    return pos;
}

static int append_uint_dec(char *buf, int pos, int max, uint32_t val) {
    char tmp[12];
    int n = 0;
    int tmp_limit = sizeof(tmp);
    if (val == 0) {
        if (pos < max - 1) {
            buf[pos++] = '0';
        }
        return pos;
    }
    while (val > 0 && n < tmp_limit) {
        tmp[n++] = '0' + (val % 10);
        val /= 10;
    }
    while (n > 0 && pos < max - 1) {
        buf[pos++] = tmp[--n];
    }
    return pos;
}

static int append_uint_hex(char *buf, int pos, int max, uint32_t val) {
    char tmp[10];
    int n = 0;
    int tmp_limit = sizeof(tmp);
    if (pos < max - 1) {
        buf[pos++] = '0';
    }
    if (pos < max - 1) {
        buf[pos++] = 'x';
    }
    if (val == 0) {
        if (pos < max - 1) {
            buf[pos++] = '0';
        }
        return pos;
    }
    while (val > 0 && n < tmp_limit) {
        uint32_t d = val & 0xF;
        if (d < 10) {
            tmp[n++] = '0' + d;
        } else {
            tmp[n++] = 'a' + (d - 10);
        }
        val >>= 4;
    }
    while (n > 0 && pos < max - 1) {
        buf[pos++] = tmp[--n];
    }
    return pos;
}

static const char *trace_syscall_name(uint32_t num) {
    switch (num) {
        case SYSCALL_WRITE:
            return "WRITE";
        case SYSCALL_YIELD:
            return "YIELD";
        case SYSCALL_GETTIME:
            return "GETTIME";
        default:
            return "UNKNOWN";
    }
}

static void trace_init_file(void) {
    g_trace_max = 0;
    g_trace_len = 0;
    if (fs_ready() == 0) {
        return;
    }
    uint32_t cap = 0;
    int cap_int = 0;
    int buf_size = sizeof(g_trace_buf);
    if (fs_file_capacity(g_trace_name8, &cap) != 0 || cap == 0) {
        return;
    }
    cap_int = cap;
    if (cap_int < buf_size) {
        g_trace_max = cap_int;
    } else {
        g_trace_max = buf_size;
    }
    memset(g_trace_buf, 0, sizeof(g_trace_buf));
    (void)fs_write_file(g_trace_name8, g_trace_buf, 0);
}

static void trace_log_line(const char *buf, int len) {
    if (buf == NULL || len <= 0) {
        return;
    }
    if (g_trace_max == 0) {
        trace_init_file();
    }
    if (g_trace_max == 0) {
        return;
    }
    if (len + 1 > g_trace_max) {
        len = g_trace_max - 1;
    }
    if (g_trace_len + len + 1 > g_trace_max) {
        g_trace_len = 0;
    }
    memcpy(g_trace_buf + g_trace_len, buf, len);
    g_trace_len += len;
    g_trace_buf[g_trace_len++] = '\n';
    (void)fs_write_file(g_trace_name8, g_trace_buf, g_trace_len);
}

void trace_set(int enabled) {
    if (enabled != 0) {
        g_trace_enabled = 1;
    } else {
        g_trace_enabled = 0;
    }
    if (g_trace_enabled != 0) {
        trace_init_file();
    }
}

int trace_get(void) {
    return g_trace_enabled;
}

static uint32_t trace_get_cs(struct IntrerruptRegisters *regs) {
    uint32_t *p = (uint32_t *)((uintptr_t)regs + sizeof(*regs));
    return p[1];
}

void trace_log_entry(const char *kind, struct IntrerruptRegisters *regs, task_t *task) {
    if (g_trace_enabled == 0 || regs == NULL) {
        return;
    }
    uint32_t cs = trace_get_cs(regs);
    uint32_t cpl = cs & 3u;
    const char *dir = "k->k";
    uint32_t pid = 0;
    if (cpl == 3) {
        dir = "u->k";
    }
    if (task != NULL) {
        pid = task->pid;
    }
    char line[80];
    int line_size = sizeof(line);
    int pos = 0;
    line[0] = 0;
    pos = append_str(line, pos, line_size, "[TRACE] ");
    pos = append_str(line, pos, line_size, dir);
    pos = append_str(line, pos, line_size, " ");
    pos = append_str(line, pos, line_size, kind);
    if (kind != NULL && kind[0] == 's' && kind[1] == 'y' && kind[2] == 's' && kind[3] == 0) {
        const char *sys_name = trace_syscall_name(regs->eax);
        pos = append_str(line, pos, line_size, " sys=");
        pos = append_str(line, pos, line_size, sys_name);
        pos = append_str(line, pos, line_size, "(");
        pos = append_uint_dec(line, pos, line_size, regs->eax);
        pos = append_str(line, pos, line_size, ")");
    }
    pos = append_str(line, pos, line_size, " int=");
    pos = append_uint_hex(line, pos, line_size, regs->int_no);
    pos = append_str(line, pos, line_size, " pid=");
    pos = append_uint_dec(line, pos, line_size, pid);
    pos = append_str(line, pos, line_size, " cpl=");
    pos = append_uint_dec(line, pos, line_size, cpl);
    line[pos] = 0;
    trace_log_line(line, pos);
}

void trace_log_return(task_t *next_task) {
    if (g_trace_enabled == 0 || next_task == NULL) {
        return;
    }
    if (next_task->is_user == 0) {
        return;
    }
    char line[80];
    int line_size = sizeof(line);
    int pos = 0;
    line[0] = 0;
    pos = append_str(line, pos, line_size, "[TRACE] k->u pid=");
    pos = append_uint_dec(line, pos, line_size, next_task->pid);
    line[pos] = 0;
    trace_log_line(line, pos);
}
