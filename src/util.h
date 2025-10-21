#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>

size_t strlen(const char *str);
void *memcpy(void* dest, const void* src, size_t n);
void *memset(void* dest, int val, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);
void *memmove(void* dest, const void* src, size_t n);
void OutPortByte(uint16_t port, uint8_t value);
uint8_t InPortByte(uint16_t port);


// struct IntrerruptRegisters {
//     uint32_t cr2;
//     uint32_t ds;
//     uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
//     uint32_t int_no, err_code;
//     uint32_t eip, cs, eflags, useresp, ss;
// };

// typedef struct IntrerruptRegisters {
//     // uint32_t cr2;                     /* topmost we pushed */

//     /* pusha order (top -> bottom): */
//     uint32_t eax;
//     uint32_t ecx;
//     uint32_t edx;
//     uint32_t ebx;
//     uint32_t esp;                     /* original esp at time of pusha */
//     uint32_t ebp;
//     uint32_t esi;
//     uint32_t edi;

//     /* saved segment registers (we pushed ds,es,fs,gs in this order) */
//     uint32_t gs;
//     uint32_t fs;
//     uint32_t es;
//     uint32_t ds;

//     /* pushed at entry (for NOERR we pushed dummy 0): */
//     uint32_t int_no;
//     uint32_t err_code;
// }__attribute__((packed));

typedef struct __attribute__((packed)) IntrerruptRegisters {
    uint32_t cr2;               // pushed after segs
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t gs, fs, es, ds;

    uint32_t int_no, err_code;
} IntrerruptRegisters;


#endif