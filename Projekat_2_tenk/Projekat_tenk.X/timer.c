#include <p30fxxxx.h>
#include "timer.h"

#define TMR1_period 50000 // HC-SR04

#define TMR2_period 250 // PWM

#define TMR3_period 10000
/*  
 * Fosc = 10MHz
 * 1/Fosc = 0.1us 
 * 0.1us * 10000 = 1ms
 * TMR5_period = 10000
 */

#define TMR4_period 50000 // HC-SR04

#define TMR5_period 10
/*  
 * Fosc = 10MHz
 * 1/Fosc = 0.1us 
 * 0.1us * 10 = 1us
 * TMR5_period = 10
 */


void Init_T1(void)
{
	TMR1 = 0;
	PR1 = TMR1_period;
	
    
	T1CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
    T1CONbits.TCKPS0 = 0;
    T1CONbits.TCKPS1 = 0; 
	//IPC1bits.T2IP = 3 // T1interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS0bits.T1IF = 0; // clear interrupt flag
	IEC0bits.T1IE = 1; // enable interrupt

	// T1CONbits.TON = 1; // T1 on
}


void Init_T2(void)
{
	TMR2 = 0;
	PR2 = TMR2_period;
	
	T2CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
	//IPC1bits.T2IP = 3 // T2 interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS0bits.T2IF = 0; // clear interrupt flag
	IEC0bits.T2IE = 1; // enable interrupt

	T2CONbits.TON = 0; // T1 off 
}


void Init_T3(void)
{
	TMR3 = 0;
	PR3 = TMR3_period;
	
    
	T3CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
    T3CONbits.TCKPS0 = 0;
    T3CONbits.TCKPS1 = 0; 
	//IPC1bits.T2IP = 3 // T1interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS0bits.T3IF = 0; // clear interrupt flag
	IEC0bits.T3IE = 1; // enable interrupt

	// T1CONbits.TON = 1; // T1 on
}


void Init_T4(void)
{
	TMR4 = 0;
	PR4 = TMR4_period;
	
    
	T4CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
    T4CONbits.TCKPS0 = 0;
    T4CONbits.TCKPS1 = 0; 
	//IPC1bits.T2IP = 3 // T1interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS1bits.T4IF = 0; // clear interrupt flag
	IEC1bits.T4IE = 1; // enable interrupt

	// T1CONbits.TON = 1; // T1 on
}


void Init_T5(void)
{
	TMR5 = 0;
	PR5 = TMR5_period;
	
	T5CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
    T5CONbits.TCKPS0 = 0;
    T5CONbits.TCKPS1 = 0; 
	//IPC1bits.T2IP = 3 // T1interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS1bits.T5IF = 0; // clear interrupt flag
	IEC1bits.T5IE = 1; // enable interrupt

	// T2CONbits.TON = 1; // T2 on 
}
