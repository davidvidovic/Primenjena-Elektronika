#ifndef PINS_H
#define	PINS_H

#include <p30fxxxx.h>

// Definisanje pinova mikrokontolera
// Navodno nije moguce koristiti makroe sa tackama u nazivu
// Ideja propada

//#define RA11        LATAbits.LATA11
//
//#define RB0         PORTBbits.RB0
//#define Rsens1      PORTBbits.RB9
//#define Rsens2      PORTBbits.RB10
//#define IN2         LATBbits.LATB1
//#define IN3         LATBbits.LATB12 
//
//#define PWM1        LATDbits.LATD0 
//#define PWM2        LATDbits.LATD1 
//#define TRIGGER_2   LATDbits.LATD2 
//#define TRIGGER_1   LATDbits.LATD3 
//#define ECHO_2      PORTDbits.RD8 
//#define ECHO_1      PORTDbits.RD9 
//
//#define IN4         LATFbits.LATF0 
//#define IN1         LATFbits.LATF1 


void InitPins();

#endif	/* PINS_H */

