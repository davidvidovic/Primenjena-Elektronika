#include <p30fxxxx.h>
#include "timer.h"

// TIMER1 radi svakih 1ms
// TIMER2 radi svake 1us

#define TMR1_period 10000 /*  Fosc = 10MHz,
					          1/Fosc = 0.1us !!!, 0.1us * 10000 = 1ms  */

#define TMR2_period 10000 /*  Fosc = 10MHz/3 = 3.33Mhz,
					          1/Fosc = 0.3us !!!, 0.3us * 3.33 = 1us  */


void Init_T1(void)
{
	TMR1 = 0;
	PR1 = TMR1_period;
	
	T1CONbits.TCS = 0; // 0 = Internal clock (FOSC/4)
	//IPC1bits.T2IP = 3 // T1interrupt pririty (0-7)
	//SRbits.IPL = 3; // CPU interrupt priority is 3(11)
	IFS0bits.T1IF = 0; // clear interrupt flag
	IEC0bits.T1IE = 1; // enable interrupt

	T1CONbits.TON = 1; // T1 on
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

	// T2CONbits.TON = 1; // T2 on 
}

