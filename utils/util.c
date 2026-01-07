#include "stdint.h"
#include "stddef.h"
#include "util.h"

size_t strlen(const char *str ){
    const char* copy = str;
    while( *copy != '\0' ){
        copy++;
    }
    return copy - str;
}

void *memcpy(void* dest, const void* src, size_t n){
    const uint8_t *source = (const uint8_t*)src;
    uint8_t *destination = (uint8_t*)dest;
    for( uint32_t i = 0 ; i < n ; ++i ){
        destination[i] = source[i];
    }
    return dest;
}

void *memset(void* dest, int val, size_t n){
    uint8_t *destination = (uint8_t*)dest;
    uint8_t byte_val = (unsigned char)val;
    for( uint32_t i = 0 ; i < n ; ++i ){
        destination[i] = byte_val;
    }
    return dest;
}

int memcmp(const void *ptr1, const void *ptr2, size_t n) {
    const unsigned char *a = (const unsigned char *)ptr1;
    const unsigned char *b = (const unsigned char *)ptr2;
    for (size_t i = 0; i < n; ++i) {
        int diff = (int)a[i] - (int)b[i];  
        if (diff) return diff;
    }
    return 0;
}

void *memmove(void* dest, const void* src, size_t n){
    uint8_t *destination = (uint8_t*)dest;
    const uint8_t *source = (const uint8_t*)src;

    if( destination < source ){
        for( size_t i = 0 ; i < n ; ++i ){
            destination[i] = source[i];
        }
    } 
    else if( destination > source ){
        for( size_t i = n ; i > 0 ; --i ){
            destination[i - 1] = source[i - 1];
        }
    }
    return dest;
}

void OutPortByte(uint16_t port, uint8_t value){
    __asm__ __volatile__ ( "outb %0, %1" : : "a"(value), "Nd"(port) );
}

uint8_t InPortByte(uint16_t port) {
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

uint16_t InPortWord(uint16_t port) {
    uint16_t val;
    __asm__ __volatile__("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
