#include <p30fxxxx.h>

void Init_T1(void);
void Init_T2(void);
void Init_T3(void);
void Init_T4(void);
void Init_T5(void);


// **************************************

// REALIZACIJA PREKIDNE RUTINE I DELAY FUNKACIJA MORA BITI U MAIN FUNKCIJI
// I ZA TIMER2 IZGLEDA OVAKO

/* 

void Delay_us(int vreme)	//funkcija za kasnjenje u mikrosekundama
{
	brojac_us = 0;
	while(brojac_us < vreme);
}


void __attribute__ ((__interrupt__)) _T2Interrupt(void) // svakih 1us
{
	TMR2 =0;   
	brojac_us++;//brojac milisekundi

	IFS0bits.T2IF = 0;    
} 

*/

// brojac_us JE GLOBALNA PROMENJIVA

// **************************************
// ZA TIMER1 IZGLEDA OVAKO

/* 

void Delay_ms(int vreme)	//funkcija za kasnjenje u milisekundama
	{
		brojac_ms = 0;
		while(brojac_ms < vreme);
	}


void __attribute__ ((__interrupt__)) _T1Interrupt(void) // svakih 1ms
{
	TMR1 =0;  
	brojac_ms++;//brojac milisekundi
  
	IFS0bits.T1IF = 0; 
} 

*/

// brojac_ms JE GLOBALNA PROMENJIVA