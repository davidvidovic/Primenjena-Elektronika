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
int brojac_ms;
int brojac_us;
int vrednost_analogni_senzor;
int vrednost_Rsens1;
int vrednost_Rsens2;

// -----------------------------------
// PREKIDNE RUTINE


// Prekidna rutina za UART1 - BLE komunikacija
void __attribute__((__interrupt__)) _U1RXInterrupt(void) 
{
    tempRX_BLE = U1RXREG;
	
    // KOD
  
    IFS0bits.U1RXIF = 0;
} 


// Prekidna rutina za UART2 - DEBUG svrhe
void __attribute__((__interrupt__)) _U2RXInterrupt(void) 
{
    tempRX_DEBUG = U2RXREG;
	
    // KOD
    
    IFS1bits.U2RXIF = 0;
} 


// Prekidna rutina za TIMER1 koji broji mikrosekunde (us)
void __attribute__ ((__interrupt__)) _T1Interrupt(void)
{
	TMR1 = 0;   
	brojac_us++;
	IFS0bits.T1IF = 0;    
}


// Prekidna rutina za TIMER2 koji broji milisekunde (ms)
void __attribute__ ((__interrupt__)) _T2Interrupt(void)
{
	TMR2 = 0;   
	brojac_ms++;
	IFS0bits.T2IF = 0;    
} 


// Prekidna rutina za AD konverziju
void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
	vrednost_analogni_senzor = ADCBUF0;
	vrednost_Rsens1 = ADCBUF1;
    vrednost_Rsens2 = ADCBUF2;
    
    // Gasimo AD konverziju
    ADCON1bits.ADON = 0;
    IFS0bits.ADIF = 0;
} 


// -----------------------------------
// POMOCNE FUNKCIJE


void Delay_us(int pauza)
{
    brojac_us = 0;
    T1CONbits.TON = 1; // T1 on
    
    while(brojac_us < pauza);
    
    T1CONbits.TON = 0; // T1 off
}


void Delay_ms(int pauza)
{
    brojac_ms = 0;
    T2CONbits.TON = 1; // T2 on
    
    while(brojac_ms < pauza);
    
    T2CONbits.TON = 0; // T2 off
}




int main(void) {
    // Inicijalizacija modula
    initUART1();
    initUART2();
    Init_T1();
    Init_T2();
    
    
    print_BLE("Initicijalizacija zavrsena!\n");
    
    // Glavna - super petlja
    while(1)
    {
        
    }
    return 0;
}
