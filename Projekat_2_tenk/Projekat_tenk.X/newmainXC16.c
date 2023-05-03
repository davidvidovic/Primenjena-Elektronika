/*
 * File:   newmainXC16.c
 * Author: david
 *
 * Created on 02. maj 2023., 09.12
 */

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <p30fxxxx.h>
#include "uart1.h"
#include "uart2.h"
#include "ADC.h"
#include "timer.h"

_FOSC(CSW_FSCM_OFF & XT_PLL4); // takt 10MHz
_FWDT(WDT_OFF); // Watch Dog Timer - OFF
//_FGS(CODE_PROT_OFF);

// -----------------------------------
// GLOBALNE PROMENJIVE

unsigned char tempRX_BLE;
unsigned char tempRX_DEBUG;
unsigned int broj1,broj2;

// -----------------------------------
// PREKIDNE RUTINE


// interrupt za UART1 - BLE komunikacija
void __attribute__((__interrupt__)) _U1RXInterrupt(void) 
{
    tempRX_BLE = U1RXREG;
	
    // KOD
  
    
    IFS0bits.U1RXIF = 0;
} 


// interrupt za UART2 - DEBUG svrhe
void __attribute__((__interrupt__)) _U2RXInterrupt(void) 
{
    tempRX_DEBUG = U2RXREG;
	
    // KOD
    
    IFS1bits.U2RXIF = 0;
} 


// -----------------------------------


int main(void) {
    // Inicijalizacija modula
    initUART1();
    initUART2();
    
    
    print_BLE("Initicijalizacija zavrsena!\n");
    
    // Glavna - super petlja
    while(1)
    {
        for(broj1=0;broj1<700;broj1++)
        for(broj2=0;broj2<300;broj2++);
        

        
        for(broj1=0;broj1<700;broj1++)
        for(broj2=0;broj2<300;broj2++);
    }
    return 0;
}
