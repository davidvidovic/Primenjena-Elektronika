#include <stdio.h>
#include <stdlib.h>
#include <p30fxxxx.h>
#include "uart1.h"
#include "ADC.h"
#include "driverGLCD.h"
#include "timer.h"

 _FOSC(CSW_FSCM_OFF & XT_PLL4); //instruction takt je isti kao i kristal
_FWDT(WDT_OFF);
//_FGS(CODE_PROT_OFF);

#define DRIVE_A LATCbits.LATC13
#define DRIVE_B LATCbits.LATC14
#define PIR_SENZOR PORTBbits.RB11
 
#define MAX_UNESENI_BROJ 4

// TIMER:
// TIMER1 - DELAY
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
// B7 - FOTOOTPORNIK
// B8 - TOUCH SCREEN?
// B9 - TOUCH SCREEN?
// B10 -  MQ3
// B11 - PIR
// B12 - SERVO
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
 
enum STATE {ZAKLJUCAN_EKRAN, POCETNI_EKRAN, POZIV, POZIV_UART};
enum STATE stanje;

int brojac_ms, brojac_us;
char tempRX;

unsigned int X, Y, x_vrednost, y_vrednost;
unsigned int sirovi0, sirovi1, sirovi2, sirovi3;
unsigned int temp0, temp1, temp2, temp3; 
int i;
unsigned char mrak;

static const unsigned char BROJ_RACUNAR[MAX_UNESENI_BROJ] = "1447";
unsigned char uneseni_broj[MAX_UNESENI_BROJ];
int indeks_uneseni_broj = 0;
unsigned char flag_TACAN_BROJ;

static const unsigned char BROJ_MIKROKONTROLER[MAX_UNESENI_BROJ] = "9876";
unsigned char uart_broj[MAX_UNESENI_BROJ];
int indeks_uart_broj = 0;

// -----------------------------------
// PREKIDNE RUTINE

void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
	sirovi0=ADCBUF0;
	sirovi1=ADCBUF1;
    sirovi2=ADCBUF2;
	sirovi3=ADCBUF3;

	temp0=sirovi0;
	temp1=sirovi1;
    temp2=sirovi2;
	temp3=sirovi3;

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
	uart_broj[indeks_uart_broj] = tempRX;
	tempRX = NULL;
    
    if(indeks_uart_broj < MAX_UNESENI_BROJ - 1)
    {
        indeks_uart_broj++;
    }
    else 
    {
        RS232_putst(uart_broj);
        indeks_uart_broj = 0;
    }
    
    IFS0bits.U1RXIF = 0;
} 


// -----------------------------------
// DELAY FUNKCIJA

void Delay_ms(int pauza)
{
    brojac_ms = 0;
    T2CONbits.TON = 1; // T2 on
    
    while(brojac_ms < pauza);
    
    T2CONbits.TON = 0; // T2 off
}

void Delay_us(int pauza)
{
    brojac_us = 0;
    T1CONbits.TON = 1; // T1 on
    
    while(brojac_us < pauza);
    
    T1CONbits.TON = 0; // T1 off
}

// -----------------------------------
// SERVO FUNKCIJE

void generisanje_PWM_servo(int pauza);
void spusti_slusalicu();
void podigni_slusalicu();

// -----------------------------------
// BUZZER FUNKCIJE

void generisanje_PWM_buzzer(int pauza);
void buzz_taster();
void buzz_otkljucavanje();
void buzz_UART_POZIV();
void buzz_pijan();

// -----------------------------------
// GLCD FUNKCIJE

void Touch_Panel(void);
void provera_pritisnutog_tastera();
static const unsigned char slika_zakljucan_ekran[1024];
static const unsigned char tastatura[1024];
static const unsigned char displej_poziv[1024];
static const unsigned char displej_poziv_uart[1024];
static const unsigned char displej_poziv_uart_prihvacen[1024];
static const unsigned char displej_poziv_uart_odbijen[1024];
static const unsigned char displej_pijan[1024];
static const unsigned char slika_zakljucan_ekran_mrak[1024];
static const unsigned char displej_pijan_mrak[1024];

// -----------------------------------
// MAIN FUNKCIJA

int main(int argc, char** argv) {
    
    // -----------------------------------
    // INICIJALIZACIJA PINOVA
    
    // BUZZER - izlaz
    TRISAbits.TRISA11 = 0;
	
	// FOTOOTPORNIK - ulaz
	ADPCFGbits.PCFG7 = 0; 
    TRISBbits.TRISB7 = 1;
    
    // MQ3
    ADPCFGbits.PCFG10 = 0; 
    TRISBbits.TRISB10 = 1;
    
    // PIR - digitalni - ulaz
    ADPCFGbits.PCFG11 = 1; 
    TRISBbits.TRISB11 = 1;
    
    // SERVO - digitalni - izlaz
    ADPCFGbits.PCFG12 = 1; 
    TRISBbits.TRISB12 = 0;

    
    // -----------------------------------
    
    Init_T1();
    Init_T2();
    
    ConfigureLCDPins();
    ConfigureTSPins();
	GLCD_LcdInit();
    
    ConfigureADCPins();
    ADCinit();
    // ADCON1bits.ADON = 1;
    
    initUART1();
    GLCD_ClrScr();
    
    stanje = ZAKLJUCAN_EKRAN;
    spusti_slusalicu();
    ADCON1bits.ADON = 0;
    mrak = 0;
	
    RS232_putst("INIT\n");
    
    
    while(1)
    {  
        // PROVEJRAVAS MQ3 I FOTOOTPORNIK
        ADCON1bits.ADON = 1;
        Delay_ms(20);
		
		// PROVEJRAVAS FOTOOTPORNIK
		if(temp0 > 2000) mrak = 1;
		else mrak = 0;
        
		// PROVEJRAVAS MQ3
        if(temp3 > 2000)
        {
            GLCD_ClrScr();
            if(mrak) GLCD_DisplayPicture(displej_pijan_mrak);
			else GLCD_DisplayPicture(displej_pijan);
            
            RS232_putst("KORISNIK JE PIJAN!\n");
            
            while(temp3 > 2000) // cekaj da se otrijezni
            {              
                buzz_pijan();
                ADCON1bits.ADON = 1;
                Delay_ms(20);
            }
            
            RS232_putst("KORISNIK SE OTRIJEZNIO!\n");
            
            stanje = ZAKLJUCAN_EKRAN;
            GLCD_ClrScr();
        }
		
       
        // PROVJERAVAS DA LI JE DOSLO DO POZIVA SA RACUNARA
        if(stanje != POZIV_UART)
        {
            flag_TACAN_BROJ = 1;
            for(i = 0; i < MAX_UNESENI_BROJ; i++)
            {
                if(uart_broj[i] != BROJ_MIKROKONTROLER[i]) flag_TACAN_BROJ = 0;
            }
            
            if(flag_TACAN_BROJ)
            {
                stanje = POZIV_UART;
                RS232_putst("POZVAN MIKROKONTROLER\n");
                GLCD_ClrScr();
                GLCD_DisplayPicture(displej_poziv_uart);
            }
        }
        
        // STATE MACHINE
        switch(stanje){
            case ZAKLJUCAN_EKRAN:
                
                if(mrak) GLCD_DisplayPicture(slika_zakljucan_ekran_mrak);
                else GLCD_DisplayPicture(slika_zakljucan_ekran);
                
                // PROVJERAVAM PIR SENZOR
                if(PIR_SENZOR)
                {
                    RS232_putst("TELEFON OTKLJUCAN POKRETOM\n");
                    buzz_otkljucavanje();
                    stanje = POCETNI_EKRAN;
                    GLCD_ClrScr();
                }
                
                // PROVJERAVAM TOUCH SCREEN
                // Ako se pritisne bilo gdje budi se ekran
                Touch_Panel();
                if(X > 0 && X < 128 && Y > 0 && Y < 64)
                {
                    RS232_putst("TELEFON OTKLJUCAN DODIROM\n");
                    buzz_otkljucavanje();
                    stanje = POCETNI_EKRAN;
                    GLCD_ClrScr();
                }
                
            break;
                
                
			case POCETNI_EKRAN:
                
				GLCD_DisplayPicture(tastatura);
				GoToXY(0, 0);
				GLCD_Printf(uneseni_broj);
                			
				// Citamo pritisnute tastere, pozivom na pravi broj : 1447 odlazimo u POZIV
				Touch_Panel();
				provera_pritisnutog_tastera();	
			break;
			
			
			case POZIV:
			
				podigni_slusalicu();
				GLCD_ClrScr();
                GLCD_DisplayPicture(displej_poziv);
                RS232_putst("MIKROKONTROLER VAS JE NAZVAO\n");
                
				for(i = 0; i < 5; i++)
                {
                    RS232_putst("...\n");
                    Delay_ms(1000);
                }
                
                RS232_putst("POZIV ZAVRSEN\n");
				spusti_slusalicu();
                uneseni_broj[0] = '\0';
				
				stanje = POCETNI_EKRAN;
                GLCD_ClrScr();
			break;
            
            
            case POZIV_UART:
			
				buzz_UART_POZIV();
				
                Touch_Panel();
                if(X > 0 && X < 64) // prihvatio poziv
                {
                    podigni_slusalicu();
                    GLCD_ClrScr();
                    GLCD_DisplayPicture(displej_poziv_uart_prihvacen);
                    RS232_putst("POZIV PRIHVACEN\n");
                    
                    for(i = 0; i < 5; i++)
                    {
                        RS232_putst("...\n");
                        Delay_ms(1000);
                    }

                    RS232_putst("POZIV ZAVRSEN\n");
                    spusti_slusalicu();
                    
                    uart_broj[0] = '\0';
                    indeks_uart_broj = 0;
                    stanje = ZAKLJUCAN_EKRAN;
                    GLCD_ClrScr();
                }
				if(X > 64 && X < 128) // odbio poziv
                {
                    RS232_putst("POZIV ODBIJEN!\n");
                    GLCD_ClrScr();
                    GLCD_DisplayPicture(displej_poziv_uart_odbijen);
                    Delay_ms(5000);
                    uart_broj[0] = '\0';
                    indeks_uart_broj = 0;
                    stanje = ZAKLJUCAN_EKRAN;
                    GLCD_ClrScr();
                }
				

			break;
		}
        
         
    }
    // return (EXIT_SUCCESS);
}

// -----------------------------------
// SERVO FUNKCIJE

void generisanje_PWM_servo(int pauza)
{
    for(i = 0; i < 5; i++)
    {
        LATBbits.LATB12 = 1;
        Delay_ms(pauza);
        LATBbits.LATB12 = 0;
        Delay_ms(20-pauza);
    }
}

void spusti_slusalicu()
{
    generisanje_PWM_servo(1);
}

void podigni_slusalicu()
{
    generisanje_PWM_servo(2);
}

// -----------------------------------
// BUZZER FUNKCIJE

void generisanje_PWM_buzzer(int pauza)
{
    LATAbits.LATA11 = 1;
    Delay_us(pauza);
    LATAbits.LATA11 = 0;
    Delay_us(500-pauza);
}

void buzz_taster()
{
    generisanje_PWM_buzzer(250);
}

void buzz_otkljucavanje()
{
    for(i = 0; i < 100; i++)
        generisanje_PWM_buzzer(50);
}

void buzz_UART_POZIV()
{
    for(i = 0; i < 100; i++)
        generisanje_PWM_buzzer(100);
}

void buzz_pijan()
{
    generisanje_PWM_buzzer(450);
}


// -----------------------------------
// GLCD FUNKCIJE

void Touch_Panel (void)
{
    
    
// vode horizontalni tranzistori
	DRIVE_A = 1;  
	DRIVE_B = 0;
    
    ADCON1bits.ADON = 1;
	Delay_ms(50); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo x	
	x_vrednost = temp1;//temp1 je vrednost koji nam daje AD konvertor na BOTTOM pinu
    X=(x_vrednost-161)*0.03629;
//    RS232_putst("Ocitao x_vrednost = ");
//    WriteUART1dec2string(x_vrednost);
//    RS232_putst("\n");
    
    
	// vode vertikalni tranzistori
    
	DRIVE_A = 0;  
	DRIVE_B = 1;
    
    ADCON1bits.ADON = 1;
	Delay_ms(50); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo y	
	y_vrednost = temp2;// temp2 je vrednost koji nam daje AD konvertor na LEFT pinu	
    
//    RS232_putst("Ocitao y_vrednost = ");
//    WriteUART1dec2string(y_vrednost);
//    RS232_putst("\n");
	
//Ako ?elimo da nam X i Y koordinate budu kao rezolucija ekrana 128x64 treba skalirati vrednosti x_vrednost i y_vrednost tako da budu u opsegu od 0-128 odnosno 0-64
//skaliranje x-koordinate

    X=(x_vrednost-250)*0.04063;
    

//X= ((x_vrednost-AD_Xmin)/(AD_Xmax-AD_Xmin))*128;	
//vrednosti AD_Xmin i AD_Xmax su minimalne i maksimalne vrednosti koje daje AD konvertor za touch panel.


//Skaliranje Y-koordinate
	Y= ((y_vrednost-450)*0.022857);

	//Y= ((y_vrednost-AD_Ymin)/(AD_Ymax-AD_Ymin))*64;
    
   
    
}

void provera_pritisnutog_tastera()
{

    if(Y > 0 && Y < 13){
		buzz_taster();
		if(X > 84 && X < 128){
			// POZOVI TASTER
            //RS232_putst("PRITISNUT TASTER POZOVI\n");
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
                    Delay_ms(4000);
                    GLCD_ClrScr();
//                    uneseni_broj[0] = '\0';
//                    indeks_uneseni_broj = 0;
                }
                else
                {
                    stanje = POZIV;
                    GLCD_ClrScr();
                }

//                indeks_uneseni_broj = 0;
//                uneseni_broj[indeks_uneseni_broj] = '\0';
//                GLCD_ClrScr();
            }
            else
            {
                GLCD_ClrScr();				
                GoToXY(0, 0);
                GLCD_Printf("NETACAN BROJ");                                      
                Delay_ms(4000);
                GLCD_ClrScr();
//                uneseni_broj[0] = '\0';
//                indeks_uneseni_broj = 0;
            }
            
            indeks_uneseni_broj = 0;
            uneseni_broj[indeks_uneseni_broj] = '\0';
            GLCD_ClrScr();
        }
    }
    
	//if(indeks_uneseni_broj + 1 < MAX_UNESENI_BROJ)
    //{
					
					// Provjera prvog reda tastature
					if(Y > 0 && Y < 13){
						buzz_taster();
						if(X > 0 && X < 42){
								// DELETE TASTER
                                //RS232_putst("PRITISNUT TASTER DELETE\n");
								if(indeks_uneseni_broj > 0){
									indeks_uneseni_broj--;
									uneseni_broj[indeks_uneseni_broj] = '\0';
								}
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                //RS232_putst("PRITISNUT TASTER 0\n");
								uneseni_broj[indeks_uneseni_broj++] = '0';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
								// POZOVI TASTER 
						}
					}
					
					// Provjera drugog reda tastature
					if(Y > 13 && Y < 26){
						buzz_taster();
						if(X > 0 && X < 42){
                                //RS232_putst("PRITISNUT TASTER 7\n");
								uneseni_broj[indeks_uneseni_broj++] = '7';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                //RS232_putst("PRITISNUT TASTER 8\n");
								uneseni_broj[indeks_uneseni_broj++] = '8';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
                                //RS232_putst("PRITISNUT TASTER 9\n");
								uneseni_broj[indeks_uneseni_broj++] = '9';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
					}
					
					
					// Provjera treceg reda tastature
					if(Y > 26 && Y < 39){
						buzz_taster();
						if(X > 0 && X < 42){
                                //RS232_putst("PRITISNUT TASTER 4\n");
								uneseni_broj[indeks_uneseni_broj++] = '4';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                //RS232_putst("PRITISNUT TASTER 5\n");
								uneseni_broj[indeks_uneseni_broj++] = '5';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
                                //RS232_putst("PRITISNUT TASTER 6\n");
								uneseni_broj[indeks_uneseni_broj++] = '6';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
					}
					
					
					// Provjera cetvrtog reda tastature
					if(Y > 39 && Y < 52){
						buzz_taster();
						if(X > 0 && X < 42){
                                //RS232_putst("PRITISNUT TASTER 1\n");
								uneseni_broj[indeks_uneseni_broj++] = '1';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                //RS232_putst("PRITISNUT TASTER 2\n");
								uneseni_broj[indeks_uneseni_broj++] = '2';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
                                //RS232_putst("PRITISNUT TASTER 3\n");
								uneseni_broj[indeks_uneseni_broj++] = '3';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
					}
			//}
//    else
//    {
//        indeks_uneseni_broj = 0;
//        //uneseni_broj[indeks_uneseni_broj] = '\0';
//    }
}



static const unsigned char slika_zakljucan_ekran[1024] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,254,255,255, 99, 99, 99, 99,  3,  0,  0,255,255,255, 
  96,240,252, 30,  7,  3,  0,  0,  0,255,255,195,195,195,227,127, 
  62, 28,  0,  0,  0,192,240,126, 15,  7, 63,252,240,128,  0,  0, 
   0,  0,255,255,  7, 30,120,224,128,  0,255,255,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,255,255,  0,  0,  0,254,255,255, 99, 99, 
  99, 99,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0, 15, 31, 31, 24, 24, 24, 24, 24,  0,  0, 31, 31, 31, 
   0,  1,  3, 15, 30, 24, 16,  0,  0, 31, 31,  0,  0,  0,  3, 15, 
  30, 24,  0, 16, 30, 31,  3,  3,  3,  3,  3,  3, 15, 31, 28,  0, 
   0,  0, 31, 31,  0,  0,  0,  1,  7, 28, 31, 31,  0,  0,  0,  0, 
   0,  0,  0, 24, 24, 24, 31, 15,  0,  0,  0, 15, 31, 31, 24, 24, 
  24, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  6,  6,198,246,126, 62, 
  14,  0,  0,  0,128,224,252, 30, 14,126,248,224,  0,  0,  0,  0, 
 254,254,254,192,224,248, 60, 14,  6,  0,  0,  0,254,254,  0,  0, 
   0,  0,  0,  0,  0,  0,254,254,  0,  0,  0,  0,254,254,  0,  0, 
   0,  0,  0,  0,254,254,  0,  0,  0,240,248,252, 14,  6,  6,  6, 
   6, 12,  0,  0,  0,128,224,252, 30, 14,126,248,224,  0,  0,  0, 
   0,  0,254,254, 14, 60,240,192,  0,  0,254,254,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0, 56, 62, 63, 51, 49, 48, 48, 
  48,  0, 32, 60, 63,  7,  6,  6,  6,  6,  7, 31, 63, 56,  0,  0, 
  63, 63, 63,  0,  3,  7, 30, 60, 48, 32,  0,  0, 63, 63, 48, 48, 
  48, 48, 48, 48, 48, 48, 63, 31,  0,  0,  0,  0, 15, 31, 56, 48, 
  48, 48, 48, 56, 31, 15,  0,  0,  0,  7, 15, 31, 56, 48, 48, 48, 
  48, 24, 24, 32, 60, 63,  7,  6,  6,  6,  6,  7, 31, 63, 56,  0, 
   0,  0, 63, 63,  0,  0,  0,  3, 15, 56, 63, 63,  0,  0,  0,  0, 
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

static const unsigned char slika_zakljucan_ekran_mrak[1024] = {
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,  1,  0,  0,156,156,156,156,252,255,255,  0,  0,  0, 
 159, 15,  3,225,248,252,255,255,255,  0,  0, 60, 60, 60, 28,128, 
 193,227,255,255,255, 63, 15,129,240,248,192,  3, 15,127,255,255, 
 255,255,  0,  0,248,225,135, 31,127,255,  0,  0,255,255,255,255, 
 255,255,255,255,255,255,  0,  0,255,255,255,  1,  0,  0,156,156, 
 156,156,252,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,240,224,224,231,231,231,231,231,255,255,224,224,224, 
 255,254,252,240,225,231,239,255,255,224,224,255,255,255,252,240, 
 225,231,255,239,225,224,252,252,252,252,252,252,240,224,227,255, 
 255,255,224,224,255,255,255,254,248,227,224,224,255,255,255,255, 
 255,255,255,231,231,231,224,240,255,255,255,240,224,224,231,231, 
 231,231,231,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,249,249,249, 57,  9,129,193, 
 241,255,255,255,127, 31,  3,225,241,129,  7, 31,255,255,255,255, 
   1,  1,  1, 63, 31,  7,195,241,249,255,255,255,  1,  1,255,255, 
 255,255,255,255,255,255,  1,  1,255,255,255,255,  1,  1,255,255, 
 255,255,255,255,  1,  1,255,255,255, 15,  7,  3,241,249,249,249, 
 249,243,255,255,255,127, 31,  3,225,241,129,  7, 31,255,255,255, 
 255,255,  1,  1,241,195, 15, 63,255,255,  1,  1,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,199,193,192,204,206,207,207, 
 207,255,223,195,192,248,249,249,249,249,248,224,192,199,255,255, 
 192,192,192,255,252,248,225,195,207,223,255,255,192,192,207,207, 
 207,207,207,207,207,207,192,224,255,255,255,255,240,224,199,207, 
 207,207,207,199,224,240,255,255,255,248,240,224,199,207,207,207, 
 207,231,231,223,195,192,248,249,249,249,249,248,224,192,199,255, 
 255,255,192,192,255,255,255,252,240,199,192,192,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};


static const unsigned char tastatura[1024] = {
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

static const unsigned char displej_poziv[1024] = 
{
   0,  0,192,192,  0,  0,  0,  0,  0,192,192,  0,  0,  0,  0,  0, 
   0,192,192,192,192,192,192,128,  0,  0,  0,128,128,192,192,192, 
 192,192,128,  0,  0,  0,192,192,192,192,192,192,  0,  0,  0,192, 
 192,  0, 64,192,128,  0,  0,  0,  0,128,192, 64,  0,192,192,  0, 
   0,  0,  0,  0,192,192,  0,  0,  0,  0,  0,  0,128,192,192,192, 
 192,128,  0,192,192,192,192,192,192,192,192,  0,192,192,192,192, 
 192,192,  0,  0,  0,  0,  0,  0,128,192,192,192,192,128,  0,  0, 
   0,  0,128,192,192,192,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,127,255,192,128,128,128,192,255,127,  0,  0,  0,  0,  0, 
   0,255,255, 24, 24, 24, 31, 15,  0,  0,126,255,193,128,128,128, 
 128,193,255,127,  0,128,192,240,184,142,135,129,128,  0,  0,255, 
 255,  0,  0,  3, 31,252,224,192,248, 31,  3,  0,  0,127,255,192, 
 128,128,128,192,255,127,  0,  0,  0,  0,  0,  0,199,143,140,156, 
 248,241,  0,  0,  0,  0,255,255,  0,  0,  0,  0,255,255,140,140, 
 140,140,  0,  0,  0,  0,  0,  0,199,143,140,156,248,241,  0,  0, 
 224,252, 63, 49, 49, 63,126,240,128,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0, 
   0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1, 
   1,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1, 
   1,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  1, 
   1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1, 
   1,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1, 
   1,  1,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,  1, 
   1,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,248,248, 24, 24, 24,184,240,224,  0,  0,  0,128,240, 56, 
  56,248,192,  0,  0,  0,224,240, 56, 24, 24, 24, 56,  0,  0,248, 
 248,  0,  0,  0,  0,  0,248,248,  0,  0,248,248, 56,240,192,  0, 
   0,248,248,  0,  0,  0,128,240, 56, 56,248,192,  0,  0,  0,248, 
 248, 24, 24, 24,184,240,224,  0,  0,192,240, 48, 24, 24, 24, 24, 
  56,240,224,  0,  0,248,248, 56,248,224,  0,  0,  0,192,248, 56, 
 248,248,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0, 63, 63,  3,  3,  3, 15, 61, 56,  0, 32, 60, 31,  7,  6, 
   6,  7, 15, 62, 48,  0, 15, 31, 56, 48, 48, 48, 48, 24,  0, 15, 
  31, 56, 48, 48, 48, 56, 31, 15,  0,  0, 63, 63,  0,  0,  3, 14, 
  56, 63, 63,  0, 32, 60, 31,  7,  6,  6,  7, 15, 62, 48,  0, 63, 
  63,  3,  3,  3, 15, 61, 56,  0,  0, 15, 31, 56, 48, 48, 48, 48, 
  24, 31, 15,  0,  0, 63, 63,  0,  0,  7, 63, 60, 62,  7,  0,  0, 
  63, 63,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
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
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const unsigned char displej_poziv_uart[1024] =
{
  0,  0,192,192,192,192,192,192,128,  0,  0,  0,128,192,192,192, 
 192,192,128,  0,  0,  0,192,192,192,192,192,192,  0,  0,192,192, 
   0, 64,192,128,  0,  0,  0,  0,192, 64,  0,  0,  0,  0,  0,192, 
 192,192,192,192,192,128,  0,  0,  0,  0,128,192,192,128,  0,  0, 
   0,  0,  0,128,192,192,192,192,128,  0,192,192,  0,  0,  0,  0, 
 192,192,  0,  0,192,192,192,128,  0,  0,  0,192,192,  0,  0,  0, 
   0,128,192,192,128,  0,  0,  0,  0,192,192,192,192,192,192,128, 
   0,  0,  0,  0,128,192,192,128,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,255,255, 12, 12, 12, 15,  7,  0,  0, 63,127,225,192,192, 
 192,225,127, 63,  0,  0,224,248,220,206,195,193,  0,  0,255,255, 
   0,  0,  3, 31,252,224,240, 63,  7,  0,  0,  0,  0,  0,  0,255, 
 255, 12, 12, 28,247,227,128,128,240,126, 55, 48, 51, 63,252,224, 
   0,  0, 63,127,225,192,192,192, 97,  0, 63,127,224,192,192,224, 
 127, 63,  0,  0,255,255,  1,  7, 14, 56,224,255,255,  0,128,240, 
 126, 55, 48, 51, 63,252,224,  0,  0,255,255, 12, 12, 28,247,227, 
 128,128,240,126, 55, 48, 51, 63,252,224,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,240,252,254, 
 254,254,254,254,252,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255, 
 255,255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,254,254, 66, 66,102, 60,  0,  0,254,254, 34, 34,254,156,  0, 
   0,254,254,  0,  0,254,254, 32, 32, 32,254,254,  0,  2, 30,252, 
 224,  0,192,120, 30,  2,  0,192,240,158,134,158,248,192,  0,  2, 
   2,  2,254,  2,  2,  2,  0,254,254,  0,  0,  0,  0,255,255,255, 
 255,255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
 248,252,  6,  2,  2,  2,222,252,  0,  0,254,254,  2,  2,  2,140, 
 252,  0,  0,254,254, 34, 34,126,220,  0,  0,254,254,  0,  0,  0, 
 254,254,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  3,  3,  0,  0,  0,  0,  0,  0,  3,  3,  0,  0,  1,  3,  2, 
   0,  3,  3,  0,  0,  3,  3,  0,  0,  0,  3,  3,  0,  0,  0,  0, 
   3,  3,  3,  0,  0,  0,  2,  3,  0,  0,  0,  0,  0,  3,  2,  0, 
   0,  0,  3,  0,  0,  0,  0,  3,  3,  0,  0,  0,  0,255,255,255, 
 255,255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   1,  1,  2,  2,  2,  2,  1,  1,  0,  0,  3,  3,  2,  2,  2,  1, 
   1,  0,  0,  3,  3,  2,  2,  3,  1,  0,  0,  3,  3,  0,  2,  2, 
   3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255, 
 255,255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255, 
 255,255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static const unsigned char displej_poziv_uart_prihvacen[1024] =
{
   0,  0,  0,  0,  0,192,192, 64, 64,192,128,  0,  0,  0,128,192, 
  64, 64, 64,192,128,  0,  0, 64, 64, 64,192,192, 64,  0,192,192, 
   0, 64,192,128,  0,  0,  0,  0,192, 64,  0,  0,  0,  0,192,192, 
  64, 64,192,128,  0,  0,192,192, 64, 64,192,128,  0,  0,192,192, 
   0,  0,192,192,  0,  0,  0,192,192,  0, 64,192,128,  0,  0,  0, 
   0,192, 64,  0,  0,  0,192,192,192,  0,  0,  0,  0,  0,128,192, 
  64, 64, 64,  0,  0,192,192, 64, 64, 64, 64,  0,192,192,192,  0, 
   0,  0,192,192,  0,  0,  0,224,224,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,127,127,  8,  8, 12,  7,  0,  0, 63, 63, 64, 
  64, 64, 64, 59, 63,  0, 64,112,120, 78, 67, 65, 64,  0,127,127, 
   0,  0,  3, 31,124, 96,120, 15,  3,  0,  0,  0,  0,  0,127,127, 
   8,  8, 12,  7,  0,  0,127,127,  4,  4, 63,115, 64,  0,127,127, 
   0,  0,127,127,  4,  4,  4,127,127,  0,  0,  3, 31,124, 96,120, 
  15,  3,  0, 64,120, 30, 19, 16, 19, 31,120, 64,  0, 63, 63, 96, 
  64, 64, 64,  0,  0,127,127, 68, 68, 68, 64,  0,127,127,  1,  7, 
  28,112,127,127,  0,  0,  0,111,103,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,128,128,  0,  0,  0,  0,128,128,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,128,128,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0, 63,127,192,128,128,128,127, 63,  0,  0, 
   0,  0,  0,254,254,  2,  2,  0,244,146,146,254,252,  0,  0,194, 
 242,158,134,  0,  0,252,162,162,182,158,  2,  0,124,198,130,130, 
 198,124,  0,  2, 30,248,192,248, 30,  2,  0,124,198,130,130,198, 
 124,  0,  0,254,254,  2,  2,  0,126,254,128,128,126,254,  0,  0, 
   0,  0,  0,158,154,146,242,  0,  2,127,255,130,130,  0,124,222, 
 146,146,150,156,  0,  0,  0,  0,  0,158,154,146,242,  0,  0,244, 
 146,146,254,252,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,192,192, 64, 64,  0,128, 
  64, 64,192,128,  3,  7,132,196, 68, 71,131,  0,192,192,  0,  0, 
 192,192,  0,  0,192,128, 64, 64,192,128,  0,  0,128, 64, 64,192, 
 128,  0,  0,192,192, 64, 64,  0,128,192, 64, 64,192,128,  0,  0, 
 192,128, 64, 64,192,128, 64, 64,192,128,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 31, 31,  0,  0,  0, 30, 
  18, 18, 31, 31,  0,  0, 15, 24, 16, 16,  8,  0, 15, 31, 16, 16, 
  15, 31,  0,  0, 31, 31,  0,  0, 31, 31,  0,  0, 30, 18, 18, 31, 
  31,  0,  0, 31, 31,  0,  0,  0, 15, 24, 16, 16, 24, 15,  0,  0, 
  31, 31,  0,  0, 31, 31,  0,  0, 31, 31,  0,  0, 24, 24,  0,  0, 
  24, 24,  0,  0, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
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

static const unsigned char displej_poziv_uart_odbijen[1024] =
{
   0,  0,  0,  0,  0,  0,  0,192,192, 64, 64,192,128,  0,  0,  0, 
 128,192, 64, 64, 64,192,128,  0,  0, 64, 64, 64,192,192, 64,  0, 
 192,192,  0, 64,192,128,  0,  0,  0,  0,192, 64,  0,  0,  0,  0, 
 192,192, 64, 64,192,128,  0,  0,  0,  0,192,192,192,  0,  0,  0, 
   0,  0,128,192, 64, 64, 64,  0,  0,192,192,  0,  0,  0,  0,192, 
 192,  0,  0,192,192,192,  0,  0,  0,192,192,  0,  0,  0,  0,192, 
 192,192,  0,  0,  0,  0,192,192, 64, 64,192,128,  0,  0,  0,  0, 
 192,192,192,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,127,127,  8,  8, 12,  7,  0,  0, 63, 
  63, 64, 64, 64, 64, 59, 63,  0, 64,112,120, 78, 67, 65, 64,  0, 
 127,127,  0,  0,  3, 31,124, 96,120, 15,  3,  0,  0,  0,  0,  0, 
 127,127,  4,  4, 63,115, 64, 64,120, 30, 19, 16, 19, 31,120, 64, 
   0, 63, 63, 96, 64, 64, 64,  0,  0, 31, 63, 96, 64, 64, 64, 63, 
  31,  0,  0,127,127,  1,  7, 28,112,127,127,  0, 64,120, 30, 19, 
  16, 19, 31,120, 64,  0,127,127,  4,  4, 63,115, 64, 64,120, 30, 
  19, 16, 19, 31,120, 64,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,128,192, 64, 64, 64,192,128,  0,  0,192,192, 64, 64, 64, 
 128,128,  0,  0,192,192, 64, 64,192,128,  0,  0,192,192,  0,  0, 
   0,192,192,  0,  0,192,192, 64, 64, 64, 64,  0,192,192,192,  0, 
   0,  0,192,192,  0,  0,  0,224,224,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0, 63, 63, 64, 64, 64, 64, 59, 63,  0,  0,127,127, 64, 64, 64, 
  49, 63,  0,  0,127,127, 68, 68,111, 59,  0,  0,127,127,  0, 64, 
  64,127, 63,  0,  0,127,127, 68, 68, 68, 64,  0,127,127,  1,  7, 
  28,112,127,127,  0,  0,  0,111,103,  0,  0,  0,  0,  0,  0,  0, 
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
   0,  0,  0,  0,  0,254,254,  0,  0,240,120, 72, 72, 88,112,  0, 
   0,120,104, 72,200,  0,  8,254,254,  8,  8,  0,240,120, 72, 72, 
  88,112,  0,  0,  0,  0,  0,255,255,  0,  0,251,251,  0,  0,248, 
 248,  0,  0,248,248,  0,  8,254,254,  8,  8,  0,251,251,  0,  0, 
   0,  0,  0,248,240,  8,  8,248,240,  0,  0,208, 72, 72,248,240, 
   0,  0,  0,  0,  0,248,240,  8,  8,248,240,  0,  0,251,251,  0, 
   0,240,120, 72, 72, 88,112,  0,  0,240,136,136,216,120,  8,  0, 
 208, 72, 72,248,240,  0,  0,  1,113,113, 27, 14,  0,  0,  0,  0, 
   0,  0,  0,  2,  2,  3,  1,  0,  0,  1,  3,  2,  2,  2,  2,  0, 
   0,  2,  2,  2,  3,  0,  0,  1,  3,  2,  2,  0,  1,  3,  2,  2, 
   2,  2,  0,  0,  0,  0,  0,  3,  3,  0, 16, 31, 15,  0,  0,  1, 
   3,  2,  2,  1,  3,  0,  0,  1,  3,  2,  2,  0,  3,  3,  0,  0, 
   0,  0,  0,  3,  3,  0,  0,  3,  3,  0,  0,  3,  2,  2,  3,  3, 
   0,  0,  0,  0,  0,  3,  3,  0,  0,  3,  3,  0, 16, 31, 15,  0, 
   0,  1,  3,  2,  2,  2,  2,  0, 12, 31, 18, 18, 18, 30, 12,  0, 
   3,  2,  2,  3,  3,  0,  0,  0,  3,  3,  0,  0,  0,  0,  0,  0 
};

static const unsigned char displej_pijan[1024] =
{
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,248,248,248, 
  56, 56, 56, 56,120,240,240,224,  0,  0,  0,248,248,248,  0,  0, 
   0,  0,  0,  0,248,248,248,  0,  0,  0,  0,  0,  0,  0,224,248, 
 120,120,248,240,128,  0,  0,  0,  0,  0,  0,248,248,248,248,248, 
 224,128,  0,  0,  0,  0,248,248,248,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,192,224,240,248, 56, 56, 56, 56,120,112,  0,  0,  0,  0, 
 248,248,248,  0,  0,  0,  0,  0,252,252,252,248,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255, 
  56, 56, 56, 56, 60, 31, 15,  7,  0,  0,  0,255,255,255,  0,  0, 
   0,  0,  0,  0,255,255,255,  0,  0,  0,  0,224,248,255,255,227, 
 224,224,227,255,255,254,240,128,  0,  0,  0,255,255,255,  0,  1, 
   7, 31,126,248,224,  0,255,255,255,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  1,  7, 15, 15, 30, 30, 60, 60,248,248,224,  0,  0,  0, 
 255,255,255,  0,  0,  0,  0,  0,  1,127,127,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15, 15, 15, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15, 15, 15,  0,  0, 
   7, 14, 14, 15, 15,  7,  3,  0,  0,  8, 15, 15, 15,  1,  0,  0, 
   0,  0,  0,  0,  0,  7, 15, 15, 12,  0,  0, 15, 15, 15,  0,  0, 
   0,  0,  0,  1,  7, 15, 15, 15, 15,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  7, 15, 14, 14, 14, 14, 14, 15,  7,  3,  1,  0,  0,  0, 
  15, 15, 15,  0,  0,  0,  0,  0,  6, 15, 15,  6,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,254, 18, 18, 30,236,  0,  0,254,254,  0, 
   0,  0,  0,252,134,  2,  2,  2,134,252,  0,  0,254, 48, 56,236, 
 134,  2,  0,254,  0,  0,254, 18, 18,114,238,  0,  0,224,124, 70, 
  78,120,192,  0,  0,254, 14, 28, 48,224,192,254,  0,  0,  0,  0, 
   2,  2,254,254,  2,  2,  0,254, 18, 18, 18,  2,  0,254,254,  0, 
   0,  0,  0,254, 18, 18, 18,  2,  0,254, 34, 34, 34,  0,  0,252, 
 134,  2,  2,  2,134,252,  0,  0,254, 14, 28, 48,224,192,254,  0, 
   0,131,191,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  1,  1,  1, 
   1,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,  0,  1,  0,  0,  0, 
   1,  1,  0,  1,  0,  0,  1,  0,  0,  0,  1,  1,  1,  1,  0,  0, 
   0,  0,  1,  1,  0,  1,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0, 
   0,  0,  1,  1,  0,  0,  0,  1,  1,  1,  1,  1,  0,  1,  1,  1, 
   1,  0,  0,  1,  1,  1,  1,  1,  0,  1,  0,  0,  0,  0,  0,  0, 
   1,  1,  1,  1,  1,  0,  0,  0,  1,  0,  0,  0,  0,  1,  1,  0, 
   0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 
};

static const unsigned char displej_pijan_mrak[1024] =
{
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,  7,  7,  7, 
 199,199,199,199,135, 15, 15, 31,255,255,255,  7,  7,  7,255,255, 
 255,255,255,255,  7,  7,  7,255,255,255,255,255,255,255, 31,  7, 
 135,135,  7, 15,127,255,255,255,255,255,255,  7,  7,  7,  7,  7, 
  31,127,255,255,255,255,  7,  7,  7,255,255,255,255,255,255,255, 
 255,255, 63, 31, 15,  7,199,199,199,199,135,143,255,255,255,255, 
   7,  7,  7,255,255,255,255,255,  3,  3,  3,  7,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,  0,  0,  0, 
 199,199,199,199,195,224,240,248,255,255,255,  0,  0,  0,255,255, 
 255,255,255,255,  0,  0,  0,255,255,255,255, 31,  7,  0,  0, 28, 
  31, 31, 28,  0,  0,  1, 15,127,255,255,255,  0,  0,  0,255,254, 
 248,224,129,  7, 31,255,  0,  0,  0,255,255,255,255,255,255,255, 
 255,255,254,248,240,240,225,225,195,195,  7,  7, 31,255,255,255, 
   0,  0,  0,255,255,255,255,255,254,128,128,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,240,240,240, 
 255,255,255,255,255,255,255,255,255,255,255,240,240,240,255,255, 
 248,241,241,240,240,248,252,255,255,247,240,240,240,254,255,255, 
 255,255,255,255,255,248,240,240,243,255,255,240,240,240,255,255, 
 255,255,255,254,248,240,240,240,240,255,255,255,255,255,255,255, 
 255,255,248,240,241,241,241,241,241,240,248,252,254,255,255,255, 
 240,240,240,255,255,255,255,255,249,240,240,249,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,  1,237,237,225, 19,255,255,  1,  1,255, 
 255,255,255,  3,121,253,253,253,121,  3,255,255,  1,207,199, 19, 
 121,253,255,  1,255,255,  1,237,237,141, 17,255,255, 31,131,185, 
 177,135, 63,255,255,  1,241,227,207, 31, 63,  1,255,255,255,255, 
 253,253,  1,  1,253,253,255,  1,237,237,237,253,255,  1,  1,255, 
 255,255,255,  1,237,237,237,253,255,  1,221,221,221,255,255,  3, 
 121,253,253,253,121,  3,255,255,  1,241,227,207, 31, 63,  1,255, 
 255,124, 64,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,254,254,254,254,255,255,255,254,254,254, 
 254,255,255,255,254,254,254,254,254,255,255,255,254,255,255,255, 
 254,254,255,254,255,255,254,255,255,255,254,254,254,254,255,255, 
 255,255,254,254,255,254,255,255,255,255,254,254,255,255,255,255, 
 255,255,254,254,255,255,255,254,254,254,254,254,255,254,254,254, 
 254,255,255,254,254,254,254,254,255,254,255,255,255,255,255,255, 
 254,254,254,254,254,255,255,255,254,255,255,255,255,254,254,255, 
 255,254,254,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, 
 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
};