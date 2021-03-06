#include <xc.h>           // processor SFR definitions
#include <sys/attribs.h>  // __ISR macro
#include <math.h>

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

#define CS LATBbits.LATB7       // chip select pin

unsigned char spi_io(unsigned char);
void spi_init();
unsigned char spi_write(unsigned short, unsigned short);

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

    spi_init();
    
    int reset_time = 24000;
    _CP0_SET_COUNT(0);
    
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    
    char tri_up = 1;
    float phase = 0;
    
    while(1) {
        if (_CP0_GET_COUNT() >= reset_time) {
            _CP0_SET_COUNT(0);
            spi_write(0, (short)(sin(phase * M_PI / 180) * 2047) + 2047);
            spi_write(1, tri_up ? phase * 4095 / 360 : 4095 - (phase * 4095 / 360));
            phase += 3.6;
            if (phase >= 356) {
                tri_up = !tri_up;
                phase = 0.0;
            }
            LATAbits.LATA4 ^= 1;
        }
	// use _CP0_SET_COUNT(0) and _CP0_GET_COUNT() to test the PIC timing
	// remember the core timer runs at half the sysclk
    }
}

unsigned char spi_io(unsigned char o) {
    SPI1BUF = o;
    while (!SPI1STATbits.SPIRBF) { // wait to receive the byte
        ;
    }
    return SPI1BUF;
}

void spi_init() {
    TRISBbits.TRISB7 = 0;
    CS = 1;

    SDI1Rbits.SDI1R = 0b0000; //A1
    RPB8Rbits.RPB8R = 0b0011; //SDO1
    
    
    // setup spi
    SPI1CON = 0; // turn off the spi module and reset it
    SPI1BUF; // clear the rx buffer by reading from it
    SPI1BRG = 0x1; // baud rate to 12 MHz [SPI4BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0; // clear the overflow bit
    SPI1CONbits.CKE = 1; // data changes when clock goes from hi to lo (since CKP is 0)
    SPI1CONbits.MSTEN = 1; // master operation
    SPI1CONbits.ON = 1; // turn on spi 1
}

unsigned char spi_write(unsigned short port, unsigned short data) {
    unsigned short spi_out = 0;
    spi_out |= (port << 15);
    spi_out |= 0x7000;
    spi_out |= (data & 0x0FFF);
    
    CS = 0;
    spi_io(spi_out >> 8);
    spi_io(spi_out & 0xFF);
    CS = 1;
}