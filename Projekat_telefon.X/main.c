//#include <stdio.h>
#include <stdlib.h>
#include <p30fxxxx.h>
#include "uart1.h"
#include "ADC.h"
#include "driverGLCD.h"
#include "timer.h"

 _FOSC(CSW_FSCM_OFF & XT_PLL4); //instruction takt je isti kao i kristal
//_FOSC(CSW_ON_FSCM_OFF & HS3_PLL4);
//_FWDT(WDT_OFF);
//_FGS(CODE_PROT_OFF);

#define DRIVE_A PORTCbits.RC13
#define DRIVE_B PORTCbits.RC14
#define MAX_UNESENI_BROJ 4

// TIMER:
// TIMER1 - GLCD
// TIMER2 - DELAY
// TIMER3 - 
//
// PORT A:
// A11 - BUZZER
//
// PORT B:
// B0 - GLCD
// B1 - GLCD
// B2 - GLCD
// B3 - GLCD
// B4 - GLCD
// B5 - 
// B6 - 
// B7 - 
// B8 - 
// B9 - 
// B10 -  
// B11 - 
// B12 - 
//
// PORT C:
// C13 - GLCD
// C14 - GLCD
// C15 - 
// 
// PORT D:
// D0 - GLCD
// D1 - GLCD
// D2 - GLCD
// D3 - GLCD
// D8 - 
// D9 - 
//
// PORT F:
// F0 - GLCD
// F1 - GLCD
// F2 - UART1
// F3 - UART1
// F4 - GLCD
// F5 - GLCD

// -----------------------------------
// GLOBALNE PROMENJIVE
 
enum STATE {ZAKLJUCAN_EKRAN, POCETNI_EKRAN, POZIV, DOLAZECI_POZIV, ODBIJEN_POZIV, SLABA_BATERIJA};
enum STATE stanje = ZAKLJUCAN_EKRAN;

long brojac_ms;
char tempRX;

unsigned int X, Y, x_vrednost, y_vrednost;
unsigned int sirovi0, sirovi1;
unsigned int temp0, temp1; 

unsigned char BROJ_RACUNAR[MAX_UNESENI_BROJ] = "1234";
unsigned char uneseni_broj[MAX_UNESENI_BROJ];
int indeks_uneseni_broj = 0;
unsigned char flag_TACAN_BROJ;

unsigned char BROJ_MIKROKONTROLER[MAX_UNESENI_BROJ] = "9876";
unsigned char uart_broj[MAX_UNESENI_BROJ];
int indeks_uart_broj = 0;


// -----------------------------------
// PREKIDNE RUTINE

void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
	sirovi0=ADCBUF0;
	sirovi1=ADCBUF1;

	temp0=sirovi0;
	temp1=sirovi1;

    IFS0bits.ADIF = 0;
} 

void __attribute__ ((__interrupt__)) _T2Interrupt(void) // svakih 1ms
{
	TMR2 = 0;   
	brojac_ms++; // brojac milisekundi

	IFS0bits.T2IF = 0;    
} 

void __attribute__((__interrupt__)) _U1RXInterrupt(void) // interrupt za UART
{
    IFS0bits.U1RXIF = 0;
    tempRX = 0;
    tempRX = U1RXREG;    
    
    uart_broj[indeks_uart_broj] = tempRX;
    
    if(indeks_uart_broj < MAX_UNESENI_BROJ)
    {
        indeks_uart_broj++;
    }
    else indeks_uart_broj = 0;
} 


// -----------------------------------
// GLCD FUNKCIJE

void Touch_Panel(void);
void provera_pritisnutog_tastera();
unsigned char slika_zakljucan_ekran[1024];
unsigned char tastatura[1024];


// -----------------------------------
// MAIN FUNKCIJA

int main(int argc, char** argv) {
    Init_T2();
    
    ConfigureLCDPins();
    ConfigureTSPins();
	GLCD_LcdInit();
    
    ConfigureADCPins();
    ADCinit();
    ADCON1bits.ADON = 1;
    
    initUART1();
    GLCD_ClrScr();

    
    while(1)
    {
        // ADC ON
        // PROVEJRAVAS MQ3
        // ADC OFF
        
        // PROVJERAVAS DA LI JE DOSLO DO POZIVA SA RACUNARA
        if(indeks_uart_broj == MAX_UNESENI_BROJ)
        {
            flag_TACAN_BROJ = 1;
            for(i = 0; i < MAX_UNESENI_BROJ; i++)
            {
                if(uart_broj[i] != BROJ_MIKROKONTROLER[i]) flag_TACAN_BROJ = 0;
            }
            
            if(flag_TACAN_BROJ)
            {
                stanje = POZIV_UART;
                GLCD_ClrScr();
            }
            else
            {
                RS232_putst("Netacan broj mikrokontrolera\n");
                indeks_uart_broj = 0;
            }
        }
        
        switch(stanje){
            case ZAKLJUCAN_EKRAN:
                
                GLCD_DisplayPicture(slika_zakljucan_ekran);
                
                // PROVJERAVAM PIR SENZOR
                
                // PROVJERAVAM TOUCH SCREEN
                Touch_Panel();
                if(X > 0 && X < 128 && Y > 0 && Y < 64)
                {
                    stanje = POCETNI_EKRAN;
                    GLCD_ClrScr();
                }
                
            break;
                
                
			case POCETNI_EKRAN:
					
				GLCD_DisplayPicture(tastatura);
				GoToXY(0, 0);
				GLCD_Printf(uneseni_broj);
				
				// Citamo pritisnute tastere, pozivom na pravi broj : 1234 odlazimo u POZIV
				Touch_Panel();
				provera_pritisnutog_tastera();
				
				
			break;
			
			
			case POZIV:
			
				podigni_slusalicu();
				GLCD_ClrScr();
                GLCD_DisplayPicture(displej_poziv);
				// DELAY 5s
				spusti_slusalicu();
				

				stanje = POCETNI_EKRAN;
                GLCD_ClrScr();
			break;
            
            
            case POZIV_UART:
			
				podigni_slusalicu();
				GLCD_ClrScr();
                GLCD_DisplayPicture(displej_poziv_uart);
				// DELAY 5s
				spusti_slusalicu();
				

				stanje = POCETNI_EKRAN;
                GLCD_ClrScr();
			break;
			
			
			
			case DOLAZECI_POZIV:
				
				
			break;
			
			case ODBIJEN_POZIV:
				
			break;
			
			
			case SLABA_BATERIJA:
				
			break;
		}
        
         
    }
    // return (EXIT_SUCCESS);
}



// -----------------------------------
// GLCD FUNKCIJE

void Touch_Panel (void)
{
// vode horizontalni tranzistori
	DRIVE_A = 1;  
	DRIVE_B = 0;
    
    // LATCbits.LATC13=1;
    // LATCbits.LATC14=0;

	//Delay(500); //cekamo jedno vreme da se odradi AD konverzija
				
	// ocitavamo x	
	x_vrednost = temp0;//temp0 je vrednost koji nam daje AD konvertor na BOTTOM pinu		

	// vode vertikalni tranzistori
    // LATCbits.LATC13=0;
    // LATCbits.LATC14=1;
	DRIVE_A = 0;  
	DRIVE_B = 1;

	//Delay(500); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo y	
	y_vrednost = temp1;// temp1 je vrednost koji nam daje AD konvertor na LEFT pinu	
	
//Ako ?elimo da nam X i Y koordinate budu kao rezolucija ekrana 128x64 treba skalirati vrednosti x_vrednost i y_vrednost tako da budu u opsegu od 0-128 odnosno 0-64
//skaliranje x-koordinate

    X=(x_vrednost-161)*0.03629;



//X= ((x_vrednost-AD_Xmin)/(AD_Xmax-AD_Xmin))*128;	
//vrednosti AD_Xmin i AD_Xmax su minimalne i maksimalne vrednosti koje daje AD konvertor za touch panel.


//Skaliranje Y-koordinate
	Y= ((y_vrednost-500)*0.020725);

//	Y= ((y_vrednost-AD_Ymin)/(AD_Ymax-AD_Ymin))*64;
}

void provera_pritisnutog_tastera()
{
	if(indeks_uneseni_broj + 1 < MAX_UNESENI_BROJ)
    {
					
					// Provjera prvog reda tastature
					if(Y > 13 && Y < 26){
						// buzz_taster();
						if(X > 0 && X < 42){
								// DELETE TASTER
								if(indeks_uneseni_broj > 0){
									indeks_uneseni_broj--;
									uneseni_broj[indeks_uneseni_broj] = "\0";
								}
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
								uneseni_broj[indeks_uneseni_broj++] = '0';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
								// POZOVI TASTER
                                if(indeks_uneseni_broj == MAX_UNESENI_BROJ)
                                {
                                    flag_TACAN_BROJ = 1;
                                    for(i = 0; i < MAX_UNESENI_BROJ; i++)
                                    {
                                        if(uneseni_broj[i] != BROJ_RACUNAR[i]) flag_TACAN_BROJ = 0;
                                    }
                                    
                                    if(!flag_TACAN_BROJ)
                                    {
                                        GLCD_ClrScr();				
                                        GoToXY(0, 0);
                                        GLCD_Printf("NETACAN BROJ");
                                        
                                        // DELAY
                                        GLCD_ClrScr();	
                                    }
                                    else
                                    {
                                        stanje = POZIV;
                                        GLCD_ClrScr();
                                    }
                                }
                                
                                indeks_uneseni_broj = 0;
                                uneseni_broj[indeks_uneseni_broj] = "\0";
                                // GLCD_ClrScr();
						}
					}
					
					// Provjera drugog reda tastature
					if(Y > 26 && Y < 39){
						// buzz_taster();
						if(X > 0 && X < 42){
								uneseni_broj[indeks_uneseni_broj++] = '7';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
								uneseni_broj[indeks_uneseni_broj++] = '8';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
								uneseni_broj[indeks_uneseni_broj++] = '9';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
					}
					
					
					// Provjera treceg reda tastature
					if(Y > 39 && Y < 52){
						// buzz_taster();
						if(X > 0 && X < 42){
								uneseni_broj[indeks_uneseni_broj++] = '4';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
								uneseni_broj[indeks_uneseni_broj++] = '5';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
								uneseni_broj[indeks_uneseni_broj++] = '6';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
					}
					
					
					// Provjera cetvrtog reda tastature
					if(Y > 52 && Y < 64){
						// buzz_taster();
						if(X > 0 && X < 42){
								uneseni_broj[indeks_uneseni_broj++] = '1';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
								uneseni_broj[indeks_uneseni_broj++] = '2';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
								uneseni_broj[indeks_uneseni_broj++] = '3';
								uneseni_broj[indeks_uneseni_broj] = "\0";
                                GLCD_ClrScr();
						}
					}
			}
    else
    {
        indeks_uneseni_broj = 0;
        uneseni_broj[indeks_uneseni_broj] = "\0";
    }
}

// ------------------------------------------------------  
// GLCD Picture name: zakljucanekran.bmp            
// GLCD Model: KS0108 128x64            
// ------------------------------------------------------  

unsigned char slika_zakljucan_ekran[1024] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,254, 18, 18, 18, 
   0,  0,254, 48, 72,134,  0,  0,254, 34, 34, 98,156,  0,  0,224, 
  92, 70, 92,224,  0,  0,254,  6, 24, 96,192,254,  0,  0,  0,  0, 
   0,  0,254,  0,  0,254, 18, 18, 18,  0,  0,  0,  0,  0,194, 34, 
  26,  6,  0,  0,224, 92, 70, 92,224,  0,  0,254, 48, 72,134,  0, 
   0,254,  0,  0,  0,  0,  0,254,  0,  0,254,128,  0,  0,  0,254, 
   0,  0,204,  2,  2,  2,  4,  0,224, 92, 70, 92,224,  0,  0,254, 
   6, 24, 96,192,254,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1, 
   0,  0,  1,  0,  0,  0,  1,  0,  1,  0,  0,  0,  1,  0,  1,  0, 
   0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0, 
   1,  1,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1, 
   1,  1,  0,  1,  0,  0,  0,  0,  0,  1,  0,  1,  0,  0,  0,  1, 
   0,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0, 
   0,  0,  0,  1,  1,  1,  0,  1,  0,  0,  0,  0,  0,  1,  0,  1, 
   0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 
};


unsigned char tastatura[1024] = {
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,128,192, 64,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0, 64,192,128,  0,  0,  0,  0,  0,128, 64,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0, 64,128,  0,  0,  0,  0,  0,128,192, 64,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 64,192,128,  0, 
   0,127,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  4,  4,  4,  2, 30, 30, 30,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,  0,255,128,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 32, 50, 50, 18, 58, 
  42, 42, 46, 38, 36,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,255,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0, 18, 18, 34, 34, 32, 38, 62, 30, 26, 
  16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,  0, 
   0,240,248,  9,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,128,128,192,192,192,192,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  1,  9,248,240,  0,  0,  0,  0,  0,240,  8,  1, 
   1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,128,192,192, 64, 
  64, 64, 64, 64,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, 
   1,  8,240,  0,  0,  0,  0,  0,240,248,  9,  1,  1,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,128,128,192,192, 64, 64, 64, 64, 64, 
  64,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  9,248,240,  0, 
   0, 31, 31, 32, 32,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2, 
   3,  3,  3,  2,  2,  3,  7,  7,  2,  2,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0, 32, 32, 31, 31,  0,  0,  0,  0,  0, 31,  0, 32, 
  32,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  2,  2,  4,  4, 
   4,  4,  3,  3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 32, 
  32,  0, 31,  0,  0,  0,  0,  0, 31, 31, 32, 32,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  3,  3,  7,  7,  5,  4,  4,  5,  7, 
   7,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0, 32, 32, 31, 31,  0, 
   0,254,255,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   8,  8, 72,104,104, 56, 24,  8,  8,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  1,255,254,  0,  0,  0,  0,  0,255,  1,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 96,120,120,152,128, 
 128,152,120,120, 96,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  1,255,  0,  0,  0,  0,  0,254,255,  1,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0, 16, 88,120,104,168,168, 40,120,120,112, 
  48,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,255,254,  0, 
   0,193,227, 32,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  4,  4, 32,227,193,  0,  0,  0,  0,  0,195, 34,  4, 
   4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4, 
   4, 34,195,  0,  0,  0,  0,  0,193,227, 32,  4,  4,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  4, 32,227,193,  0, 
   0, 63,127,  0,128,  0,  0,  0,  0,  0,  0,  0,  0,  1, 25,  9, 
  11, 14, 14,  6,  4,  6,  6, 14, 11,  9, 25,  1,  0,  0,  0,  0, 
   0,  0,  0,  0,128,  0,127,127,  0,  0,  0,  0,  0,127,  0,128, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 14, 15, 15, 25, 17, 
  17, 17, 15, 15, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
 128,  0,127,  0,  0,  0,  0,  0,127,127,  0,128,  0,  0,  0,  0, 
   0,  0,  0,  0,  8,  8,  8,  8, 24, 24, 24, 12, 12,  6,  6,  3, 
   3,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,128,  0,127,127,  0 
};