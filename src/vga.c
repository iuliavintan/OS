#include"vga.h"
#include"stdint.h"

//current column and line i am at
uint16_t column = 0;
uint16_t line = 0;
uint16_t *const vga = (uint16_t*const) 0xB8000; //sets location to the video mem
const uint16_t defaultColour = (COLOUR8_BLACK << 8) | (COLOUR8_LIGHT_GREY <<12);
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