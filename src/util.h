#include <stddef.h>

size_t strlen(const char *str);
void *memcpy(void* dest, const void* src, size_t n);
void *memset(void* dest, int val, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t n);
void *memmove(void* dest, const void* src, size_t n);
void OutPortByte(uint16_t port, uint8_t value);
