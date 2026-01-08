#include "gdt.h"
#include "utils/util.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t gran;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss_entry {
    uint32_t prev;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr gdtp;
static struct tss_entry tss;

extern void gdt_flush(struct gdt_ptr *gdtp);

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[idx].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[idx].base_low = (uint16_t)(base & 0xFFFF);
    gdt[idx].base_mid = (uint8_t)((base >> 16) & 0xFF);
    gdt[idx].access = access;
    gdt[idx].gran = (uint8_t)((limit >> 16) & 0x0F);
    gdt[idx].gran |= gran & 0xF0;
    gdt[idx].base_high = (uint8_t)((base >> 24) & 0xFF);
}

static void tss_write(uint32_t base, uint32_t limit) {
    gdt_set_entry(5, base, limit, 0x89, 0x00);
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = KERNEL_DS;
    tss.iomap = sizeof(tss);
}

void gdt_init(void) {
    gdtp.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtp.base = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xCF);
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);

    tss_write((uint32_t)&tss, sizeof(tss));

    gdt_flush(&gdtp);

    asm volatile("ltr %%ax" :: "a"(0x28));
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}
