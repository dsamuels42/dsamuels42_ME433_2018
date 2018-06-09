#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<stdio.h>
#include "ST7735.h"

// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // no boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable secondary osc
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576 // use slowest wdt
#pragma config WINDIS = OFF // wdt no window mode
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz
#pragma config UPLLIDIV = DIV_2 // divider for the 8MHz input clock, then multiplied by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // USB BUSON controlled by USB module

void display_char(char, unsigned short, unsigned short, 
                  unsigned short, unsigned short);
void draw_string(unsigned short, unsigned short, char*, 
                 unsigned short, unsigned short);
void draw_bar(unsigned short, unsigned short, unsigned char, 
              unsigned char, unsigned short, unsigned short);


int main() {

    __builtin_disable_interrupts();

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here

    __builtin_enable_interrupts();

    int reset_time = 24000000/10;
    _CP0_SET_COUNT(0);
    
    LCD_init();
    LCD_clearScreen(BLACK);
    int progress = 0;
    int time = 0;
    
    while(1) {
        if ((time = _CP0_GET_COUNT()) >= reset_time) {
            //time = _CP0_GET_COUNT();
            _CP0_SET_COUNT(0);
            char time_out[3];
            time_out[1] = 0;
            sprintf(time_out, "%d ", reset_time * 10 / time);
            draw_string(0, 0, time_out, CYAN, BLACK);
            char message[26];
            sprintf(message, "Hello world %d!", progress);
            draw_string(28, 32, message, CYAN, BLACK);
            draw_bar(28, 45, progress*3/4, 75, CYAN, MAGENTA);
            progress = (progress + 1) % 100;
        }
        
	// use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing
	// remember the core timer runs at half the sysclk
    }
}

void display_char(char c, unsigned short x_init, unsigned short y_init, 
                  unsigned short color, unsigned short bg_color) {
    const char* c_vals = ASCII[c-0x20];
    unsigned short x, y, draw_color;
    for(x = 0; x < 5; x++) {
        if(x + x_init > 128) break;
        for(y = 0; y < 8; y++) {
            if(y + y_init > 160) break;
            draw_color = (c_vals[x]&(1<<y)) ? color : bg_color;
            LCD_drawPixel(x+x_init, y+y_init, draw_color);
        }
    }
}

void draw_string(unsigned short x, unsigned short y, char* message, 
                 unsigned short color, unsigned short bg_color) {
    unsigned mess_pos = 0;
    while(message[mess_pos]) {
        display_char(message[mess_pos], x+5*mess_pos, y, color, bg_color);
        mess_pos++;
    }
}

void draw_bar(unsigned short x, unsigned short y, unsigned char len, 
              unsigned char total_len, unsigned short color, unsigned short bg_color) {
    int i;
    unsigned short draw_color;
    for(i = 0; i < total_len; i++) {
        draw_color = i < len ? color : bg_color;
        LCD_drawPixel(x+i, y, draw_color);
        LCD_drawPixel(x+i, y + 1, draw_color);
    }
}