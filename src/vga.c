#include "vga.h"
#include"stdint.h"
#include "util.h"

//current column and line i am at
uint16_t column = 0;
uint16_t line = 0;
uint16_t *const vga = (uint16_t*const) 0xB8000; //sets location to the video mem
const uint16_t defaultColour = (COLOUR8_LIGHT_GREY << 8) | (COULOUR8_MANGENTA <<12);
uint16_t currentColour = defaultColour; //variable in case we want to change the colour

int cursor_x, cursor_y;
const uint16_t defaultColour_kern = (COLOUR8_LIGHT_BLUE << 8) | (COLOUR8_BLACK<<12);
uint16_t currentColour_kern = defaultColour_kern; //variable in case we want to change the colour

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
    if(line<vga_height-1)
    {
        line++;
        column=0;
    }
    else
    {
        //we are at the bottom of the screen
        scroll_up();
        column=0;
    }
    cursor_y+=1;
    cursor_x = 0;
    update_cursor();
}

void scroll_up()
{
    for(uint16_t y=0; y<vga_height; y++)
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
static void kputc(char c, uint16_t currentColour){
    switch(c)
        {
            case'\n':
                new_line();
                break;
            case '\r':
                column=0;
                cursor_x = 0;
                cursor_y = line;
                update_cursor();
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
                update_cursor();
                break;
            case '\b':
                if( column > 0 ){
                    column--;
                    vga[line * vga_width + column] = ' ' | currentColour;
                    cursor_x = column;
                    cursor_y = line;
                    update_cursor();
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
                update_cursor();
                break;
        }
}


void print(const char *s)
{
    while( *s) { putc(*s++);}
               
}

void kprint(const char *s)
{
    while(*s){ kputc((*s++),currentColour_kern);}
}

void putc(char c)
{
    switch(c)
        {
            case'\n':
                new_line();
                break;
            case '\r':
                column=0;
                cursor_x = 0;
                cursor_y = line;
                update_cursor();
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
                update_cursor();
                break;
            case '\b':
                if( column > 0 ){
                    column--;
                    vga[line * vga_width + column] = ' ' | currentColour;
                    cursor_x = column;
                    cursor_y = line;
                    update_cursor();
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
                update_cursor();
                break;
        }
}
void disable_cursor()
{
	OutPortByte(0x3D4, 0x0A);
	OutPortByte(0x3D5, 0x20);
}
void update_cursor()
{
	uint16_t pos = cursor_y * vga_width + cursor_x;

	OutPortByte(0x3D4, 0x0F);
	OutPortByte(0x3D5, (uint8_t) (pos & 0xFF));
	OutPortByte(0x3D4, 0x0E);
	OutPortByte(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
void get_cursor_position(int *x, int *y)
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