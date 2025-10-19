#ifndef VGA_H
#define VGA_H

#include"stdint.h"

#define COLOUR8_BLACK 0
#define COLOUR8_LIGHT_GREY 7    

#define vga_width 80
#define vga_height 25

void print(const char* s);
void scroll_up();
void new_line();
void reset();

#endif