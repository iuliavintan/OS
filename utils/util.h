#ifndef UTIL_H
#define UTIL_H
#include "stddef.h"
#include "stdint.h"

size_t strlen(const char *str);
void *memcpy(void* dest, const void* src, size_t n);
void *memset(void* dest, int val, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);
void *memmove(void* dest, const void* src, size_t n);
void OutPortByte(uint16_t port, uint8_t value);
void OutPortWord(uint16_t port, uint16_t value);
uint8_t InPortByte(uint16_t port);
uint16_t InPortWord(uint16_t port);


typedef struct __attribute__((packed)) IntrerruptRegisters {
    uint32_t cr2;                     // top
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
} IntrerruptRegisters;


#endif
