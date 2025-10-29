#ifndef VGA_H
#define VGA_H

#include"stdint.h"

#define COLOUR8_BLACK 0
#define COLOUR8_BLUE 1
#define COLOUR8_GREEN 2
#define COLOUR8_CYAN 3
#define COLOUR8_RED 4
#define COULOUR8_MANGENTA 5
#define COLOUR8_BROWN 6
#define COLOUR8_LIGHT_GREY 7    
#define COLOUR8_DARK_GREY 8
#define COLOUR8_LIGHT_BLUE 9
#define COLOUR8_LIGHT_GREEN 10
#define COLOUR8_LIGHT_CYAN 11
#define COLOUR8_LIGHT_RED 12
#define COLOUR8_LIGHT_MANGENTA 13
#define COLOUR8_LIGHT_BROWN 14
#define COLOUR8_WHITE 15

#define vga_width 80
#define vga_height 25

void print(const char* s);
void scroll_up();
void new_line();
void reset();
void putc(char c);
void set();
void disable_cursor();
void update_cursor(uint16_t cursor_x, uint16_t cursor_y);
void get_cursor_position(uint16_t *x, uint16_t *y);
void kprint(const char *s);
void deletec(uint16_t *cursor_x, uint16_t *cursor_y);

#endif