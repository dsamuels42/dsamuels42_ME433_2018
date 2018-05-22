#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include "i2c_master_noint.h"  //i2c library

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
unsigned char i2c_read();
void initExp();

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
    
    
    //initialize i2c and port extender
    initExp();
    
    //setup for heartbeat
    int reset_time = 24000000/5/2;
    _CP0_SET_COUNT(0);
    
    //setup io pins
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    
    while(1) {
        //enable heartbeat at 5Hz
        if (_CP0_GET_COUNT() >= reset_time) {
            _CP0_SET_COUNT(0);
            PORTAINV = 0x00000010;
        }
        
        if(i2c_read() & 0x80) {
            i2c_write(0x0A, 0x0F);
        } else {
            i2c_write(0x0A, 0x00);
        }
        
    }
}

void initExp() {
    //disable analog input for i2c pins
    ANSELBbits.ANSB2 = 0;
    ANSELBbits.ANSB3 = 0;
    
    //setup for 12c at 400kHz
    i2c_master_setup();
    
    i2c_write(0x00, 0xF0);
    i2c_write(0x0A, 0x0F);
}

#define ADDR 0b0100000

void i2c_write(unsigned char reg, unsigned char val) {
    i2c_master_start();
    i2c_master_send(ADDR<<1|0);
    i2c_master_send(reg);
    i2c_master_send(val);
    i2c_master_stop();
}

unsigned char i2c_read() {
    i2c_master_start();
    i2c_master_send(ADDR<<1|0);
    i2c_master_send(0x09);
    i2c_master_restart();
    i2c_master_send(ADDR<<1|1);
    unsigned char r = i2c_master_recv();
    i2c_master_stop();
    return r;
}

