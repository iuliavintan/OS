#define VIDERO_MEMORY 0xB8000
#define WHITE_ON_BLACK 0x07

void print(const char *str) {
   unsigned short *vga = (unsigned short *)VIDERO_MEMORY;
   while(*str) {
      *vga++ = (*str++ | (WHITE_ON_BLACK << 8));
   }
}

void main(void){
    print("Hello from C in protected mode!\n");
    while(1);

}