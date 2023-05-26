#include "pins.h"
#include <p30fxxxx.h>

void InitPins()
{
    // Inicjalizacija ulazno/izlaznih pinova
    
    TRISAbits.TRISA11 = 0;  // Ugradjena LED dioda
    
    TRISBbits.TRISB0 = 1;   // Analogni senzor
    // TRISBbits.TRISB1 = 1;   // Analogna letvica
    // TRISBbits.TRISB2 = 1;   // Analogna letvica
    // TRISBbits.TRISB3 = 1;   // Analogna letvica
    // TRISBbits.TRISB4 = 1;   // Analogna letvica
    // TRISBbits.TRISB5 = 1;   // Analogna letvica
    TRISBbits.TRISB9 = 1;   // Rsens1
    TRISBbits.TRISB10 = 1;  // Rsens2
    TRISBbits.TRISB11 = 0;  // IN2 drajvera
    TRISBbits.TRISB12 = 0;  // IN3 drajvera
    
    TRISDbits.TRISD0 = 0;   // PWM1
    TRISDbits.TRISD1 = 0;   // PWM2
    TRISDbits.TRISD2 = 0;   // Trigger za HCSR2
    TRISDbits.TRISD3 = 0;   // Trigger za HCSR1
    TRISDbits.TRISD8 = 1;   // Echo za HCSR2
    TRISDbits.TRISD9 = 1;   // Echo za HCSR1
    
    TRISFbits.TRISF0 = 0;   // IN4 drajvera
    TRISFbits.TRISF1 = 0;   // IN1 drajvera
    
    
    // Inicijalizacija analognih pinova
    
    ADPCFGbits.PCFG0 = 0;
    // ADPCFGbits.PCFG1 = 0;
    // ADPCFGbits.PCFG2 = 0;
    // ADPCFGbits.PCFG3 = 0;
    // ADPCFGbits.PCFG4 = 0;
    // ADPCFGbits.PCFG5 = 0;
    ADPCFGbits.PCFG9 = 0;
    ADPCFGbits.PCFG10 = 0;
    ADPCFGbits.PCFG11 = 1;
    ADPCFGbits.PCFG12 = 1;
}