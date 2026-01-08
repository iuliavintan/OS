#include "process.h"
#include "fs/fs.h"
#include "memory/kheap.h"
#include "memory/vmm.h"
#include "utils/util.h"
#include "utils/vga.h"

#define USER_BASE       0x00400000u
#define USER_STACK_TOP  0x00800000u
#define USER_STACK_SIZE 0x2000u

#define EI_NIDENT 16
#define PT_LOAD 1
#define PF_W 2

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) Elf32_Phdr;

static process_t *g_proc_list = NULL;

void process_init(void) {
    g_proc_list = NULL;
}

static int elf_valid(const Elf32_Ehdr *eh) {
    if (!eh) return 0;
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F') {
        return 0;
    }
    if (eh->e_ident[4] != 1 || eh->e_ident[5] != 1) {
        return 0;
    }
    if (eh->e_phnum == 0 || eh->e_phentsize != sizeof(Elf32_Phdr)) {
        return 0;
    }
    return 1;
}

static int map_user_range(uint32_t *pd, uint32_t vaddr, uint32_t size, uint32_t flags) {
    uint32_t start = vaddr & 0xFFFFF000u;
    uint32_t end = (vaddr + size + 0xFFFu) & 0xFFFFF000u;
    for (uint32_t va = start; va < end; va += PAGE_SIZE) {
        uintptr_t frame = pmm_alloc_page();
        if (!frame) {
            return -1;
        }
        if (vmm_map_page_pd(pd, va, frame, flags) != 0) {
            return -1;
        }
    }
    return 0;
}

static int load_elf_from_fs(const char *name8, uint32_t *entry_out,
                            uint32_t *pd, uintptr_t pd_phys, uintptr_t kernel_pd_phys) {
    uint32_t sectors = 0;
    uint32_t max_sectors = 256;
    uint32_t buf_bytes = max_sectors * 512;
    uint8_t *buf = (uint8_t *)kmalloc(buf_bytes);
    if (!buf) {
        print("exec: out of memory\n");
        return -1;
    }

    if (fs_load_file(name8, buf, max_sectors, &sectors) != 0) {
        kfree(buf);
        return -1;
    }

    uint32_t file_bytes = sectors * 512;
    if (file_bytes < sizeof(Elf32_Ehdr)) {
        kfree(buf);
        return -1;
    }

    Elf32_Ehdr *eh = (Elf32_Ehdr *)buf;
    if (!elf_valid(eh)) {
        kfree(buf);
        return -1;
    }

    if (eh->e_phoff + (eh->e_phnum * sizeof(Elf32_Phdr)) > file_bytes) {
        kfree(buf);
        return -1;
    }

    Elf32_Phdr *ph = (Elf32_Phdr *)(buf + eh->e_phoff);
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) {
            continue;
        }
        if (ph[i].p_vaddr < USER_BASE) {
            uint32_t end = ph[i].p_vaddr + ph[i].p_memsz;
            if (ph[i].p_offset == 0 && end <= USER_BASE && ph[i].p_filesz <= 0x1000u) {
                continue;
            }
            kfree(buf);
            return -1;
        }
        uint32_t flags = VMM_FLAG_PRESENT | VMM_FLAG_USER;
        if (ph[i].p_flags & PF_W) {
            flags |= VMM_FLAG_RW;
        }
        if (map_user_range(pd, ph[i].p_vaddr, ph[i].p_memsz, flags) != 0) {
            kfree(buf);
            return -1;
        }
        if (ph[i].p_offset + ph[i].p_filesz > file_bytes) {
            kfree(buf);
            return -1;
        }
    }

    vmm_switch_pd(pd_phys);
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) {
            continue;
        }
        memcpy((void *)ph[i].p_vaddr, buf + ph[i].p_offset, ph[i].p_filesz);
        if (ph[i].p_memsz > ph[i].p_filesz) {
            memset((void *)(ph[i].p_vaddr + ph[i].p_filesz), 0,
                   ph[i].p_memsz - ph[i].p_filesz);
        }
    }
    vmm_switch_pd(kernel_pd_phys);

    *entry_out = eh->e_entry;
    kfree(buf);
    return 0;
}

static void free_user_address_space(task_t *t) {
    if (!t || !t->page_dir_phys) {
        return;
    }
    if (t->page_dir_phys == vmm_kernel_pd_phys()) {
        return;
    }
    uint32_t *pd = (uint32_t *)t->page_dir_phys;
    uint32_t user_pdi_start = USER_BASE >> 22;
    uint32_t user_pdi_end = (USER_STACK_TOP - 1) >> 22;
    for (uint32_t pdi = user_pdi_start; pdi <= user_pdi_end; pdi++) {
        uint32_t pde = pd[pdi];
        if (!(pde & VMM_FLAG_PRESENT)) {
            continue;
        }
        uint32_t *pt = (uint32_t *)(pde & 0xFFFFF000u);
        for (uint32_t pti = 0; pti < 1024; pti++) {
            uint32_t pte = pt[pti];
            if (pte & VMM_FLAG_PRESENT) {
                uintptr_t frame = pte & 0xFFFFF000u;
                pmm_free_page(frame);
            }
        }
        pmm_free_page((uintptr_t)pt);
        pd[pdi] = 0;
    }
    pmm_free_page(t->page_dir_phys);
    t->page_dir_phys = vmm_kernel_pd_phys();
}

process_t *process_exec(const char *name8) {
    if (!fs_ready()) {
        print("exec: fs not ready\n");
        return NULL;
    }

    uintptr_t pd_phys = 0;
    uint32_t *pd = vmm_create_user_pd(&pd_phys);
    if (!pd) {
        print("exec: pd alloc failed\n");
        return NULL;
    }
    uint32_t user_pdi_start = USER_BASE >> 22;
    uint32_t user_pdi_end = (USER_STACK_TOP - 1) >> 22;
    for (uint32_t pdi = user_pdi_start; pdi <= user_pdi_end; pdi++) {
        pd[pdi] = 0;
    }

    uint32_t entry = 0;
    if (load_elf_from_fs(name8, &entry, pd, pd_phys, vmm_kernel_pd_phys()) != 0) {
        print("exec: load failed\n");
        return NULL;
    }

    uint32_t stack_base = USER_STACK_TOP - USER_STACK_SIZE;
    if (map_user_range(pd, stack_base, USER_STACK_SIZE, VMM_FLAG_PRESENT | VMM_FLAG_USER | VMM_FLAG_RW) != 0) {
        print("exec: stack map failed\n");
        return NULL;
    }

    task_t *t = task_create_user(entry, USER_STACK_TOP);
    if (!t) {
        print("exec: task create failed\n");
        return NULL;
    }

    process_t *p = (process_t *)kmalloc(sizeof(process_t));
    if (!p) {
        print("exec: out of memory\n");
        return NULL;
    }
    memset(p, 0, sizeof(*p));
    p->pid = t->pid;
    p->state = PROC_READY;
    p->cpu_ticks = 0;
    memcpy(p->name, name8, 8);
    p->name[8] = 0;
    p->task = t;
    p->next = g_proc_list;
    g_proc_list = p;

    t->proc = p;
    t->page_dir_phys = pd_phys;
    return p;
}

void process_kill(uint32_t pid) {
    process_t *p = g_proc_list;
    while (p) {
        if (p->pid == pid) {
            p->state = PROC_ZOMBIE;
            if (p->task) {
                p->task->state = TASK_ZOMBIE;
            }
            return;
        }
        p = p->next;
    }
    print("kill: pid not found\n");
}

void process_reap_task(task_t *t) {
    if (!t) {
        return;
    }
    process_t *prev = NULL;
    process_t *p = g_proc_list;
    while (p) {
        if (p->task == t) {
            if (prev) {
                prev->next = p->next;
            } else {
                g_proc_list = p->next;
            }
            free_user_address_space(t);
            kfree(p);
            return;
        }
        prev = p;
        p = p->next;
    }
    free_user_address_space(t);
}

void process_list(void) {
    process_t *p = g_proc_list;
    print("PID  STATE   CPU  NAME\n");
    while (p) {
        const char *state = "READY";
        if (p->state == PROC_RUNNING) state = "RUN";
        if (p->state == PROC_ZOMBIE) state = "ZOMB";
        print("%d  %s   %d  %s\n", p->pid, state, p->cpu_ticks, p->name);
        p = p->next;
    }
}

void process_on_tick(task_t *current) {
    if (!current || !current->proc) {
        return;
    }
    process_t *p = (process_t *)current->proc;
    p->cpu_ticks++;
    p->state = PROC_RUNNING;
}
