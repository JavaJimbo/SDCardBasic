/* SDCardDemo for UBW32 or SNAD PIC boards - See USE_UBW32 in Defs.h
 * NO USB Version
 * 
 * Compiled for PIC32MX795F512L using XC32 V1.30 compiler
 * MPLABX Version 4.10
 *  
 * 8-5-18 JBS: Adapted from SD Pots project
 *        Updated Defs.h to work for SNAD PIC or UBW32 boards.
 */
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "FSIO.h"
#include "Delay.h"
#include "Defs.h"

#include "HardwareProfile.h"
#include "GenericTypeDefs.h"
#include "Compiler.h"

/** CONFIGURATION **************************************************/
#pragma config UPLLEN   = ON        // USB PLL Enabled
#pragma config FPLLMUL  = MUL_15        // PLL Multiplier
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select


unsigned char HOSTRxBuffer[MAXBUFFER+1];
unsigned char HOSTTxBuffer[MAXBUFFER+1];
unsigned char HOSTRxBufferFull = FALSE;

void InitializeSystem(void);


void main(void) 
{
unsigned short i=0, numBytes;
FSFILE *filePtr;
char filename[] = "TestFile.txt";
int length;
char ch;

    InitializeSystem();
    DelayMs(100);
#ifdef USE_UBW32    
    printf("\r>>Starting SD card demo for UBW32 BOARD");
#else     
    printf("\r>>Starting SD card demo for SNAD PIC BOARD");
#endif
    printf("\rWait for Media Detect...");
    while (!MDD_MediaDetect());     // Wait for SD detect to go low    
    printf("\rInitializing SD card...");
    while (!FSInit());
    printf("\rOpening test file...");
    filePtr = FSfopen(filename, FS_READ);            
    if (filePtr==NULL) printf("Error: could not open %s", filename);
    else
    {
        printf("\rSuccess! Opened %s. Reading data\r", filename);    
        do {           
            numBytes = FSfread(&ch, 1, 1, filePtr);
            putchar(ch);
        } while (numBytes);    
        printf("\rClosing file");
        FSfclose(filePtr); 
        printf("\rDONE");        
    }
    DelayMs(10);        // Initialize SD card    
    
    while(1)
    {
        DelayMs(1);    
        if (HOSTRxBufferFull)
        {
            HOSTRxBufferFull = false;        
            printf("\rOpening %s to append...", filename);
            filePtr = FSfopen(filename, FS_APPEND);
            length = strlen(HOSTRxBuffer);
            printf("\rWriting %d bytes...", length);        
            numBytes = FSfwrite(HOSTRxBuffer, 1, length, filePtr);
            printf("\r%d bytes written", numBytes);
            FSfclose(filePtr); 
            printf("\rDONE");            
        }                
    }
}


void InitializeSystem(void) 
{	   
    char ch;
    SYSTEMConfigPerformance(60000000);
    
    // UserInit();
    // Turn off JTAG so we get the pins back
    mJTAGPortEnable(false);
    
    // Set up HOST UART    
    UARTConfigure(HOSTuart, UART_ENABLE_HIGH_SPEED | UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode(HOSTuart, UART_INTERRUPT_ON_TX_DONE | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(HOSTuart, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
#define SYS_FREQ 60000000
    UARTSetDataRate(HOSTuart, SYS_FREQ, 57600);
    UARTEnable(HOSTuart, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    // Configure HOST UART Interrupts
    INTEnable(INT_SOURCE_UART_TX(HOSTuart), INT_DISABLED);
    INTEnable(INT_SOURCE_UART_RX(HOSTuart), INT_ENABLED);
    INTSetVectorPriority(INT_VECTOR_UART(HOSTuart), INT_PRIORITY_LEVEL_2);
    INTSetVectorSubPriority(INT_VECTOR_UART(HOSTuart), INT_SUB_PRIORITY_LEVEL_0);
    
    if (UARTReceivedDataIsAvailable(HOSTuart)) 
        ch = UARTGetDataByte(HOSTuart);
    
    // Turn on the interrupts
    INTEnableSystemMultiVectoredInt();

}//end UserInit


void __ISR(HOST_VECTOR, ipl2) IntHostUartHandler(void) {
static unsigned short HOSTRxIndex = 0;
unsigned char ch;    
    
    if (HOSTbits.OERR || HOSTbits.FERR){
        if (UARTReceivedDataIsAvailable(HOSTuart)) 
            ch = UARTGetDataByte(HOSTuart);   
        HOSTbits.OERR=0;
        HOSTRxIndex=0;
    }
    else if (INTGetFlag(INT_SOURCE_UART_RX(HOSTuart))) {
        INTClearFlag(INT_SOURCE_UART_RX(HOSTuart));
        if (UARTReceivedDataIsAvailable(HOSTuart)) {
            ch = UARTGetDataByte(HOSTuart);
            if(HOSTRxIndex < MAXBUFFER)
                HOSTRxBuffer[HOSTRxIndex++] = ch;                
            if (ch=='\r')
            {
                HOSTRxBuffer[HOSTRxIndex] = '\0';  // $$$$
                HOSTRxBufferFull = true;
                HOSTRxIndex=0;
            }                   
        }
    }
    if (INTGetFlag(INT_SOURCE_UART_TX(HOSTuart))) 
        INTClearFlag(INT_SOURCE_UART_TX(HOSTuart));    

}


