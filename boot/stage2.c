// #define VIDEO_MEMORY 0xB8000
// #define WHITE_ON_BLACK 0x07

// void print(const char *str) {
//    unsigned short *vga = (unsigned short *)VIDEO_MEMORY;
//    while(*str) {
//       *vga++ = (*str++ | (WHITE_ON_BLACK << 8));
//    }
// }

// void main(void){
//     print("Hello from C in protected mode!\n");
//     while(1);

// }

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;

#define VIDEO_MEMORY ((volatile uint16_t*)0xB8000)
#define WHITE_ON_BLACK 0x07
static inline uint16_t vga_entry(char ch, uint8_t attr){ return (uint16_t)ch | ((uint16_t)attr<<8); }

void print(const char* s){
    volatile uint16_t* vga = VIDEO_MEMORY;
    while(*s){
        *vga++ = vga_entry(*s++, WHITE_ON_BLACK);
    }
}

void main(void){
   //cli();
    print("Hello from C in protected mode!");
    for(;;)__asm__ __volatile__("hlt");
}
