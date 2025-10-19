#include "vga.h"
#include"stdint.h"

//current column and line i am at
uint16_t column = 0;
uint16_t line = 0;
uint16_t *const vga = (uint16_t*const) 0xB8000; //sets location to the video mem
const uint16_t defaultColour = (COLOUR8_LIGHT_GREY << 8) | (COULOUR8_MANGENTA <<12);
uint16_t currentColour = defaultColour; //variable in case we want to change the colour


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

void print(const char *s)
{
    while(*s)
    {
        switch(*s)
        {
            case'\n':
                new_line();
                break;
            case '\r':
                column=0;
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
                break;
            default:
                if(column == vga_width)
                {
                    new_line();

                }
                vga[line*vga_width + (column++)] = *s | currentColour;
                break;
        }
        s++;
    }
}