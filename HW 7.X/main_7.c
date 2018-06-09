#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include "i2c_master_noint.h"  //i2c library
#include "ST7735.h"
#include<limits.h>

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

void i2c_write(unsigned char, unsigned char);
unsigned char i2c_read(unsigned char);
void initMEMS();
void i2c_read_multiple(unsigned char, unsigned char*, int);
void draw_bar(unsigned short, unsigned short, char, 
              char, unsigned char, unsigned char,
              unsigned short, unsigned short);
void display_char(char, unsigned short, unsigned short, 
                  unsigned short, unsigned short);

#define XMAX 128
#define YMAX 160
#define XSCALE (SHRT_MAX/XMAX/2)
#define YSCALE (SHRT_MAX/YMAX/2)

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

    __builtin_enable_interrupts();
    
  
    LCD_init();
    LCD_clearScreen(GREEN);
    
    //initialize i2c and port extender
    initMEMS();
    
    //setup for heartbeat
    int reset_time = 24000000/20;
    _CP0_SET_COUNT(0);
    
    //setup io pins
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    

    
    while(1) {
        //enable heartbeat at 10Hz
        if (_CP0_GET_COUNT() >= reset_time) {
            _CP0_SET_COUNT(0);
            LATAbits.LATA4 ^= 1;
            
            char vals[14];
            i2c_read_multiple(0x20, vals, 14); //read 14 bytes starting at OUT_TEMP_L
            short temp =    vals[0] | (vals[1] << 8);
            short gyroX =   vals[2] | (vals[3] << 8);
            short gyroY =   vals[4] | (vals[5] << 8);
            short gyroZ =   vals[6] | (vals[7] << 8);
            short accelX =  vals[8] | (vals[9] << 8);
            short accelY =  vals[10] | (vals[11] << 8);
            short accelZ =  vals[12] | (vals[13] << 8);
            draw_bar(XMAX/2, YMAX/2, accelX / XSCALE, accelY / YSCALE, XMAX/2, YMAX/2, BLACK, GREEN);
        }
    }
}

void initMEMS() {
    //disable analog input for i2c pins
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;
    
    //setup for 12c at 400kHz
    i2c_master_setup();
    
    unsigned char whoami = i2c_read(0x0F);
    while(whoami != 0x69) {LATAbits.LATA4 = 1;}
    
    i2c_write(0x10, 0x82); //CTRL1_XL
    i2c_write(0x11, 0x88); //CTRL2_G
    i2c_write(0x12, 0x04); //CTRL3_C
}

#define ADDR 0b1101011

void i2c_write(unsigned char reg, unsigned char val) {
    i2c_master_start();
    i2c_master_send(ADDR<<1|0);
    i2c_master_send(reg);
    i2c_master_send(val);
    i2c_master_stop();
}

unsigned char i2c_read(unsigned char reg) {
    i2c_master_start();
    i2c_master_send(ADDR<<1|0);
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(ADDR<<1|1);
    unsigned char r = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    return r;
}

void i2c_read_multiple(unsigned char reg, unsigned char *data, int length) {
    i2c_master_start();
    i2c_master_send(ADDR<<1|0);
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(ADDR<<1|1);
    int i;
    for(i = 0; i < length; i++) {
        data[i] = i2c_master_recv();
        i2c_master_ack(i == length - 1 );
    }
    i2c_master_stop();
}

void draw_bar(unsigned short x, unsigned short y, char x_len, 
              char y_len, unsigned char x_total, unsigned char y_total, 
              unsigned short color, unsigned short bg_color) {
    int i;
    unsigned short draw_color;
    for(i = 0; i < x_total; i++) {
        draw_color = (i < x_len) && (x_len > 0) ? color : bg_color;
        LCD_drawPixel(x+i, y, draw_color);
        LCD_drawPixel(x+i, y + 1, draw_color);
    }
    for(i = 0; i > -x_total; i--) {
        draw_color = (i > x_len) && (x_len < 0) ? color : bg_color;
        LCD_drawPixel(x+i, y, draw_color);
        LCD_drawPixel(x+i, y + 1, draw_color);
    }
    for(i = 0; i < y_total; i++) {
        draw_color = (i < y_len) && (y_len > 0) ? color : bg_color;
        LCD_drawPixel(x, y+i, draw_color);
        LCD_drawPixel(x+1, y+i, draw_color);
    }
    for(i = 0; i > -y_total; i--) {
        draw_color = (i > y_len) && (y_len < 0) ? color : bg_color;
        LCD_drawPixel(x, y+i, draw_color);
        LCD_drawPixel(x+1, y+i, draw_color);
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

