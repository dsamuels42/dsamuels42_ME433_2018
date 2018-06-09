/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "ST7735.h"
#include "i2c_master_noint.h"
#include <limits.h>

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


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


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    appData.state = APP_STATE_INIT;

    LCD_init();
    LCD_clearScreen(GREEN);
    
    initMEMS();
    
    _CP0_SET_COUNT(0);
    
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    /* Check the application's current state. */
    switch ( appData.state )
    {
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
       
        
            if (appInitialized)
            {
                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }
        case APP_STATE_SERVICE_TASKS:
        {
            int reset_time = 24000000/20;
            
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
        /* The default state should never be executed. */
        default:
        {
            break;
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


/*******************************************************************************
 End of File
 */
