#include<p30fxxxx.h>

void ConfigureADCPins(void);

void ADCinit_analog(void);

void ADCinit(void);


// ******************************

// U MAIN FUNKCIJI NAM TREBA U SETUP DIJELU:

// DCON1bits.ADON = 1;

// OZNACAVA POCETAK AD KONVERZIJE



// ******************************

// U MAIN FUNKCIJI NAM TREBA PREKIDNA RUTINA

/* 

void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
	sirovi0 = ADCBUF0;
								
    IFS0bits.ADIF = 0;

} 

 */
 
 // GDJE JE int sirovi0 GLOBALNA PROMENJIVA

