#include <stddef.h>

size_t strlen(const char *str);
void *memcpy(void* dest, const void* src, size_t n);
void *memset(void* dest, int val, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);
void *memmove(void* dest, const void* src, size_t n);
void OutPortByte(uint16_t port, uint8_t value);


struct IntrerruptRegisters {
    uint32_t cr2;
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};