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
#include "ADC.h"
#include "timer.h"

 _FOSC(CSW_FSCM_OFF & XT_PLL4); // takt 10MHz
_FWDT(WDT_OFF); // Watch Dog Timer - OFF
//_FGS(CODE_PROT_OFF);

// -----------------------------------
// PREKIDNE RUTINE

void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
	sirovi0=ADCBUF0;
	sirovi1=ADCBUF1;
    sirovi2=ADCBUF2;
	sirovi3=ADCBUF3;

    ADCON1bits.ADON = 0;
    IFS0bits.ADIF = 0;
} 

void __attribute__ ((__interrupt__)) _T1Interrupt(void) // svakih 1us
{
	TMR1 = 0;   
	brojac_us++; // brojac mikrosekundi
	IFS0bits.T1IF = 0;    
} 
  
void __attribute__ ((__interrupt__)) _T2Interrupt(void) // svakih 1ms
{
	TMR2 = 0;   
	brojac_ms++; // brojac milisekundi
	IFS0bits.T2IF = 0;    
} 

void __attribute__((__interrupt__)) _U1RXInterrupt(void) // interrupt za UART
{
    tempRX = U1RXREG;
	
    // KOD
    
    IFS0bits.U1RXIF = 0;
} 

int main(void) {
    return 0;
}
