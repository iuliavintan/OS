#include "vga.h"
#include"stdint.h"
#include "util.h"
#include<stdarg.h>

//current column and line i am at
uint16_t column = 0;
uint16_t line = 0;
uint16_t *const vga = (uint16_t*const) 0xB8000; //sets location to the video mem
const uint16_t defaultColour = (COLOUR8_LIGHT_GREY << 8) | (COLOUR8_BLACK <<12);
uint16_t currentColour = defaultColour; //variable in case we want to change the colour

uint16_t cursor_x, cursor_y;
const uint16_t defaultColour_kern = (COLOUR8_LIGHT_MANGENTA<< 8) | (COLOUR8_BLACK<<12);
uint16_t currentColour_kern = defaultColour_kern; //variable in case we want to change the colour

uint16_t vga_make_color(uint8_t fg, uint8_t bg) {
    return (uint16_t)((fg << 8) | (bg << 12));
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    currentColour = vga_make_color(fg, bg);
}

void vga_set_kernel_color(uint8_t fg, uint8_t bg) {
    currentColour_kern = vga_make_color(fg, bg);
}

void vga_reset_color(void) {
    currentColour = defaultColour;
}

void vga_reset_kernel_color(void) {
    currentColour_kern = defaultColour_kern;
}

//initialises the screen
void reset()
{
    line=0;
    column=0;
    currentColour=defaultColour;

    for(uint16_t y=0; y<vga_height; y++)
    {
        for(uint16_t x=0; x<vga_width; x++)
        {
            vga[y *vga_width + x] = ' ' | defaultColour;
        }
    }
}

void set(){
    line=0;
    column=0;
    currentColour_kern=defaultColour_kern;

        for(uint16_t x=0; x<vga_width; x++)
        {
            vga[vga_width + x] = ' ' | defaultColour_kern;
        }
    
}

void new_line()
{
    if (line < vga_height - 1) {
        line++;
    } else {
        // We are at the bottom of the screen, keep cursor on last row.
        scroll_up();
        line = vga_height - 1;
    }
    column = 0;
    cursor_y = line;
    cursor_x = 0;
    update_cursor(cursor_x, cursor_y);
}

void scroll_up()
{
    for(uint16_t y=1; y<vga_height; y++)
    {
        for(uint16_t x=0; x<vga_width; x++)
        {
            vga[(y-1)*vga_width+x] = vga[y*vga_width+x];
        }
    }

    for(uint16_t x=0; x<vga_width; x++)
    {
        vga[(vga_height-1)*vga_width+x]=' ' | currentColour;
    }
}
void deletec(uint16_t *column, uint16_t *line){
    if( *column > 0 ){
        (*column)--;
        vga[*line * vga_width + *column] = ' ' | currentColour;
        cursor_x = *column;
        //*column = cursor_x;
        cursor_y = *line;
        // *line = cursor_y;
        update_cursor(cursor_x,cursor_y);
    }
}
static void kputc(char c, uint16_t currentColour){
    get_cursor_position(&cursor_x,&cursor_y);
    column = cursor_x;
    line = cursor_y;
    switch(c)
        {
            case'\n':
                new_line();
                break;
            case '\r':
                column=0;
                cursor_x = 0;
                cursor_y = line;
                update_cursor(cursor_x,cursor_y );
                break;
            case '\t':
                if(column==vga_width)
                {
                    new_line();
                }
                uint16_t tabLen = 4 - (column % 4); //
                while(tabLen)
                {
                    vga[line*vga_width + (column++)] = ' ' | currentColour;
                    tabLen--;
                } 
                cursor_x = column;
                cursor_y = line;
                update_cursor(cursor_x,cursor_y);
                break;
            case '\b':
                deletec(&column,&line);
                break;

            default:
                if(column == vga_width)
                {
                    new_line();

                }
                vga[line*vga_width + (column++)] = c | currentColour;
                cursor_x = column;
                cursor_y = line;
                update_cursor(cursor_x,cursor_y);
                break;
        }
}

void print_decimal(int val){
    char buff[16];
    int i=0;
    if(val==0){
        putc('0');
        return;
    }
    if(val<0){
        putc('-');
        val=-val;
    }
    while(val>0){
        buff[i++]='0'+(val%10);
        val=val/10;
    }
    while(i--){
        putc(buff[i]);
    }
}

void kprint_decimal(int val,uint16_t currentColour){
    char buff[16];
    int i=0;
    if(val==0){
        kputc('0',currentColour);
        return;
    }
    if(val<0){
        kputc('-',currentColour);
        val=-val;
    }
    while(val>0){
        buff[i++]='0'+(val%10);
        val=val/10;
    }
    while(i--){
        kputc(buff[i],currentColour);
    }
}

void print_hex(uint32_t x) {
    const char *hex = "0123456789abcdef";
    putc('0'); putc('x');
    for (int i = 7; i >= 0; --i)
        putc(hex[(x >> (i*4)) & 0xF]);
}

void kprint_hex(uint32_t x,uint16_t currentColour) {
    const char *hex = "0123456789abcdef";
    kputc('0', currentColour); kputc('x',currentColour);
    for (int i = 7; i >= 0; --i)
        kputc(hex[(x >> (i*4)) & 0xF],currentColour);
}

void print(const char *s,...)
{
    va_list args;
    va_start(args, s);

    while( *s){
        if(*s=='%'){
            s++;
            switch(*s){
                case 'd':   //int
                    print_decimal(va_arg(args, int));
                    break;
                case 'c':   //char
                    putc((char)va_arg(args, int));
                    break;
                case 'x':
                    print_hex(va_arg(args, unsigned));
                    break;
                case 's': {  //string
                    char *str = va_arg(args, char *);
                    while(*str){
                        putc(*str++);
                    }
                }
                    break;
                case '%':   //actual procent
                    putc('%');
                    break;
                default:
                    putc('%');
                    putc(*s);
                    break;
            }
        }
        else{
            putc(*s);
        }
        s++;
    }
    
    va_end(args);
}
void print_error(const char *s,...)
{
    va_list args;
    va_start(args, s);
    uint16_t prevColour = currentColour;
    currentColour = (COLOUR8_LIGHT_RED << 8) | (COLOUR8_BLACK <<12);
    while( *s){
        if(*s=='%'){
            s++;
            switch(*s){
                case 'd':   //int
                    kprint_decimal(va_arg(args, int),currentColour);
                    break;
                case 'c':   //char
                    kputc((char)va_arg(args, int),currentColour);
                    break;
                case 'x':
                    kprint_hex(va_arg(args, unsigned),currentColour);
                    break;
                case 's': {  //string
                    char *str = va_arg(args, char *);
                    while(*str){
                        kputc(*str++,currentColour);
                    }
                }
                    break;
                case '%':   //actual procent
                    kputc('%',currentColour);
                    break;
                default:
                    kputc('%',currentColour);
                    kputc(*s,currentColour);
                    break;
            }
        }
        else{
            kputc(*s,currentColour);
        }
        s++;
    }
    currentColour = prevColour;
    va_end(args);
}
void kprint(const char *s,...)
{
    va_list args;
    va_start(args, s);

    while( *s){
        if(*s=='%'){
            s++;
            switch(*s){
                case 'd':   //int
                    kprint_decimal(va_arg(args, int),defaultColour_kern);
                    break;
                case 'c':   //char
                    kputc((char)va_arg(args, int),defaultColour_kern);
                    break;
                case 'x':
                    kprint_hex(va_arg(args, unsigned),defaultColour_kern);
                    break;
                case 's': {  //string
                    char *str = va_arg(args, char *);
                    while(*str){
                        kputc(*str++,defaultColour_kern);
                    }
                }
                    break;
                case '%':   //actual procent
                    kputc('%',defaultColour_kern);
                    break;
                default:
                    kputc('%',defaultColour_kern);
                    kputc(*s,defaultColour_kern);
                    break;
            }
        }
        else{
            kputc(*s,defaultColour_kern);
        }
        s++;
    }
    
    va_end(args);
}

void putc(char c)
{
    get_cursor_position(&cursor_x,&cursor_y);
    column = cursor_x;
    line = cursor_y;
    switch(c)
        {
            case'\n':
                new_line();
                break;
            case '\r':
                column=0;
                cursor_x = 0;
                cursor_y = line;
                update_cursor(cursor_x,cursor_y);
                break;
            case '\t':
                if(column==vga_width)
                {
                    new_line();
                }
                uint16_t tabLen = 4 - (column % 4); //
                while(tabLen)
                {
                    vga[line*vga_width + (column++)] = ' ' | currentColour;
                    tabLen--;
                } 
                cursor_x = column;
                cursor_y = line;
                update_cursor(cursor_x,cursor_y);
                break;
            case '\b':
                if( column > 0 ){
                    column--;
                    vga[line * vga_width + column] = ' ' | currentColour;
                    cursor_x = column;
                    cursor_y = line;
                    update_cursor(cursor_x,cursor_y);
                }
                break;

            default:
                if(column == vga_width)
                {
                    new_line();

                }
                vga[line*vga_width + (column++)] = c | currentColour;
                cursor_x = column;
                cursor_y = line;
                update_cursor(cursor_x,cursor_y);
                break;
        }
}
void disable_cursor()
{
	OutPortByte(0x3D4, 0x0A);
	OutPortByte(0x3D5, 0x20);
}
void update_cursor(uint16_t cursor_x, uint16_t cursor_y)
{   
    if( cursor_x >= 0 && cursor_x <= vga_width ){
        uint16_t pos = cursor_y * vga_width + cursor_x;

        OutPortByte(0x3D4, 0x0F);
        OutPortByte(0x3D5, (uint8_t) (pos & 0xFF));
        OutPortByte(0x3D4, 0x0E);
        OutPortByte(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
    }
}
void get_cursor_position(uint16_t *x, uint16_t *y)
{
    uint16_t pos = 0;
    OutPortByte(0x3D4, 0x0F);
    pos |= InPortByte(0x3D5);
    OutPortByte(0x3D4, 0x0E);
    pos |= ((uint16_t)InPortByte(0x3D5)) << 8;
    *y = pos / vga_width;
    *x = pos % vga_width;
    //return pos;
}
