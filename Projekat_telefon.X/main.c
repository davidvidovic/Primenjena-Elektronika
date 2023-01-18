#include <stdio.h>
#include <stdlib.h>
#include <p30fxxxx.h>
#include "uart1.h"
#include "ADC.h"
#include "driverGLCD.h"
#include "timer.h"

 _FOSC(CSW_FSCM_OFF & XT_PLL4); //instruction takt je isti kao i kristal
//_FOSC(CSW_ON_FSCM_OFF & HS3_PLL4);
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
// B7 - 
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
 
enum STATE {ZAKLJUCAN_EKRAN, POCETNI_EKRAN, POZIV, POZIV_UART, SLABA_BATERIJA};
enum STATE stanje;

int brojac_ms, brojac_us;
char tempRX;

unsigned int X, Y, x_vrednost, y_vrednost;
unsigned int sirovi0, sirovi1, sirovi2;
unsigned int temp0, temp1, temp2; 
int i;

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

	temp0=sirovi0;
	temp1=sirovi1;
    temp2=sirovi2;

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
    uart_broj[indeks_uart_broj] = U1RXREG;
    
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
    
//    RS232_putst("UART_BROJ: ");
//    RS232_putst(uart_broj);
//    RS232_putst("   I indeks je ");
//    WriteUART1dec2string(indeks_uart_broj);
//    RS232_putst("\n");
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

// -----------------------------------
// MAIN FUNKCIJA

int main(int argc, char** argv) {
    
    // -----------------------------------
    // INICIJALIZACIJA PINOVA
    
    // BUZZER - izlaz
    TRISAbits.TRISA11 = 0;
    
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
    
    RS232_putst("INIT\n");
    
//    indeks_uneseni_broj = 4;
    
    while(1)
    {  
        
        
        
        // PROVEJRAVAS MQ3
//        ADCON1bits.ADON = 1;
//        Delay_ms(20);
//        
//        if(temp2 > 2000)
//        {
//            GLCD_ClrScr();
//            GLCD_DisplayPicture(displej_pijan);
//            
//            RS232_putst("KORISNIK JE PIJAN!\n");
//            
//            while(temp2 > 2000) // cekaj da se otrijezni
//            {              
//                buzz_pijan();
//                ADCON1bits.ADON = 1;
//                Delay_ms(20);
//            }
//            
//            RS232_putst("KORISNIK SE OTRIJEZNIO!\n");
//            
//            stanje = ZAKLJUCAN_EKRAN;
//            GLCD_ClrScr();
//        }
       
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
                
                spusti_slusalicu();
                GLCD_DisplayPicture(slika_zakljucan_ekran);
                
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
                
				podigni_slusalicu();
                
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
                
                // AKO BUDE SKAKAO NEGDJE DRUGO SAMO STAVI DELAY BEY FORA
				for(i = 0; i < 5; i++)
                {
                    RS232_putst("...\n");
                    Delay_ms(1000);
                }
                // Delay_ms(5000);
                
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
                    
                    // AKO BUDE SKAKAO NEGDJE DRUGO SAMO STAVI DELAY BEY FORA
                    for(i = 0; i < 5; i++)
                    {
                        RS232_putst("...\n");
                        Delay_ms(1000);
                    }
                    // Delay_ms(5000);

                    RS232_putst("POZIV ZAVRSEN\n");
                    spusti_slusalicu();
                    
                    uart_broj[0] = '\0';
                    indeks_uart_broj = 0;
                    stanje = ZAKLJUCAN_EKRAN;
                    GLCD_ClrScr();
                }
				if(X > 64 && X < 128) // odbio poziv
                {
                    GLCD_ClrScr();
                    GLCD_DisplayPicture(displej_poziv_uart_odbijen);
                    Delay_ms(5000);
                    uart_broj[0] = '\0';
                    indeks_uart_broj = 0;
                    stanje = ZAKLJUCAN_EKRAN;
                    GLCD_ClrScr();
                }
				

			break;
			
			case SLABA_BATERIJA:
				
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
    Delay_us(1000-pauza);
}

void buzz_taster()
{
    generisanje_PWM_buzzer(500);
}

void buzz_otkljucavanje()
{
    for(i = 0; i < 100; i++)
        generisanje_PWM_buzzer(100);
}

void buzz_UART_POZIV()
{
    generisanje_PWM_buzzer(700);
}

void buzz_pijan()
{
    generisanje_PWM_buzzer(900);
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
	x_vrednost = temp0;//temp0 je vrednost koji nam daje AD konvertor na BOTTOM pinu
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
	y_vrednost = temp1;// temp1 je vrednost koji nam daje AD konvertor na LEFT pinu	
    
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
//    RS232_putst("Ocitao x_vrednost = ");
//    WriteUART1dec2string(x_vrednost);
//    RS232_putst("    X = ");
//    WriteUART1dec2string(X);
//    RS232_putst("\n");
//        
//    RS232_putst("Ocitao y_vrednost = ");
//    WriteUART1dec2string(y_vrednost);
//    RS232_putst("    Y = ");
//    WriteUART1dec2string(Y);
//    RS232_putst("\n");
    
    if(Y > 0 && Y < 13){
		buzz_taster();
		if(X > 84 && X < 128){
			// POZOVI TASTER
            RS232_putst("PRITISNUT TASTER POZOVI\n");
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
                                RS232_putst("PRITISNUT TASTER DELETE\n");
								if(indeks_uneseni_broj > 0){
									indeks_uneseni_broj--;
									uneseni_broj[indeks_uneseni_broj] = '\0';
								}
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                RS232_putst("PRITISNUT TASTER 0\n");
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
                                RS232_putst("PRITISNUT TASTER 7\n");
								uneseni_broj[indeks_uneseni_broj++] = '7';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                RS232_putst("PRITISNUT TASTER 8\n");
								uneseni_broj[indeks_uneseni_broj++] = '8';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
                                RS232_putst("PRITISNUT TASTER 9\n");
								uneseni_broj[indeks_uneseni_broj++] = '9';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
					}
					
					
					// Provjera treceg reda tastature
					if(Y > 26 && Y < 39){
						buzz_taster();
						if(X > 0 && X < 42){
                                RS232_putst("PRITISNUT TASTER 4\n");
								uneseni_broj[indeks_uneseni_broj++] = '4';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                RS232_putst("PRITISNUT TASTER 5\n");
								uneseni_broj[indeks_uneseni_broj++] = '5';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
                                RS232_putst("PRITISNUT TASTER 6\n");
								uneseni_broj[indeks_uneseni_broj++] = '6';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
					}
					
					
					// Provjera cetvrtog reda tastature
					if(Y > 39 && Y < 52){
						buzz_taster();
						if(X > 0 && X < 42){
                                RS232_putst("PRITISNUT TASTER 1\n");
								uneseni_broj[indeks_uneseni_broj++] = '1';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 42 && X < 84){
                                RS232_putst("PRITISNUT TASTER 2\n");
								uneseni_broj[indeks_uneseni_broj++] = '2';
								//uneseni_broj[indeks_uneseni_broj] = '\0';
                                GLCD_ClrScr();
						}
						
						if(X > 84 && X < 128){
                                RS232_putst("PRITISNUT TASTER 3\n");
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
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127, 
 192,128,128,192,127,  0,  0,  0,  0,  0,255, 17, 17, 17, 14,  0, 
   0,126,194,129,129,129, 67,126,  0,128,225,177,137,135,129,  0, 
   0,  0,  0,  1, 14,112,192,112, 14,  1,  0,127,192,128,128,192, 
 127,  0,  0,  0,  0,  0,143,137,145,240,  0,  1,  1,  1,  1,  1, 
   1,  0,255,137,137,137,129,  0,  0,  0,  0,143,137,145,240,  0, 
 128, 96, 44, 35, 39, 56,192,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,128, 
 128,128,128,  0,  0,  0,  0,  0,128,128,  0,  0,  0,  0,128,128, 
 128,128,  0,  0,128,  0,  0,  0,  0,128,  0,  0,128,128,  0,  0, 
   0,128,  0,  0,  0,  0,128,128,  0,  0,  0,128,128,128,128,  0, 
   0,  0,  0,  0,128,128,128,128,  0,  0,  0,128,128,  0,  0,  0, 
   0,  0,128,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127, 
   8,  8, 24,103,  0, 64, 48, 22, 17, 19, 28, 96,  0, 63, 97, 64, 
  64, 64, 32,  0, 63, 96, 64, 64, 96, 63,  0,  0,127,  1,  6, 24, 
 112,127,  0, 64, 48, 22, 17, 19, 28, 96,  0,127,  8,  8, 24,103, 
   0,  0, 63, 97, 64, 64, 64, 33, 63,  0,  0,127,  1,  6, 56, 96, 
  24,  6,  1,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
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
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 
};

static const unsigned char displej_poziv_uart[1024] =
{
  0,  0,  0,  0,  0,  0,  0,  0,  0, 32, 32, 32,224, 32,  0,192, 
  64, 32, 32, 32, 96,192,  0, 32,192,  0,  0,  0,192, 32,  0,224, 
  32, 32, 32, 32,  0,  0,  0, 32,192,  0,  0,  0,192, 32,  0,  0, 
 128, 96,224,  0,  0,  0,224, 32, 32,  0,  0,  0,  0,  0,  0,224, 
  32, 32, 32,192,  0,  0,  0,128, 96,224,  0,  0,  0,192, 96, 32, 
  32, 32,  0,  0,224,  0,  0,  0,  0,224,  0,  0,224, 96,128,  0, 
   0,224,  0,  0,  0,128, 96,224,  0,  0,  0,224, 32, 32, 32,192, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0, 16, 28, 22, 17, 16, 16,  0, 15, 
  24, 16, 16, 16,  8, 15,  0,  0,  1, 14, 24, 14,  1,  0,  0, 31, 
  17, 17, 17, 16,  0,  0,  0,  0,  1, 14, 24, 14,  1,  0, 16, 12, 
   5,  4,  4,  7, 24,  0, 17, 17, 18, 30,  0,  0,  0,  0,  0,159, 
 194,194,198,153,  0, 16, 12,  5,  4,  4,  7, 24,  0, 15, 24, 16, 
  16, 16,  8,  0, 15, 24, 16, 16, 24, 15,  0,  0, 31,  0,  1,  6, 
  28, 31,  0, 16, 12,  5,  4,  4,  7, 24,  0, 31,  2,  2,  6, 25, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 
 255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 
 255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,254, 34, 34, 34, 28,  0,  0,254, 34, 34, 98,156, 
   0,  0,  0,  0,  0,  0,254, 16, 16, 16, 16,254,  0,  2, 28,224, 
 128,224, 28,  2,  0,192, 88, 70, 78,112,128,  2,  2,  2,  2,  2, 
   2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 
 255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
 126,194,129,129,129, 67,126,  0,  0,255,129,129,129,195,126,  0, 
   0,255,137,137,137,118,  0,  0,  0,  0,  0,128,128,127,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1, 
   0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1,  0,  0,  0,  0, 
   1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 
 255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 
 255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 
 255,255,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 
};

static const unsigned char displej_poziv_uart_prihvacen[1024] =
{
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,240, 16, 16, 16,224, 
   0,  0,240, 16, 16, 16,224,  0,  0,  0,  0,  0,  0,240,128,128, 
 128,128,240,  0, 16,224,  0,  0,  0,224, 16,  0,  0,192, 48,112, 
 128,  0,  0,224, 48, 16, 16, 16,  0,  0,240,144,144,144, 16,  0, 
 240, 48,192,  0,  0,240,  0,  0,  0,  0,  0,240, 16, 16, 16,224, 
   0,  0,224, 32, 16, 16, 16, 48,224,  0,  0, 16, 16,144,112, 16, 
   0,  0,  0,  0, 16,224,  0,  0,  0,224, 16,  0,  0,248,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  1,  1,  1,  0, 
   0,  0, 15,  1,  1,  3, 12,  0,  0,  0,  0,  0,  0, 15,  0,  0, 
   0,  0, 15,  0,  0,  0,  7, 12,  7,  0,  0,  8,  6,  2,  2,  2, 
   3, 12,  0,  7, 12,  8,  8,  8,  4,  0, 15,  8,  8,  8,  8,  0, 
  15,  0,  0,  3, 14, 15,  0,  0,  0,  0,  0, 15,  1,  1,  1,  0, 
   0,  0,  7, 12,  8,  8,  8,  4,  7,  0,  8, 14, 11,  8,  8,  8, 
   0,  0,  0,  0,  0,  0,  7, 12,  7,  0,  0,  0,  0, 11,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,224, 32, 32, 32,192,  0,  0,  0,192, 96,192, 
   0,  0,  0, 32, 32,160, 96,  0,  0,192, 64, 32, 32, 32, 64,  0, 
   0,192, 32, 32, 32, 32,192,  0, 32,192,  0,  0,  0,192, 32,  0, 
   0,192, 96,192,  0,  0,  0,224, 32, 32, 32,192,  0,  0,  0,192, 
  96,192,  0,  0, 32, 32, 32, 32, 32, 32,  0,224, 32, 32, 32,  0, 
   0,  0,  0,  0,224, 32, 32,  0,  0,  0,  0,192, 96,192,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0, 31,  2,  2,  6, 25,  0, 16, 14,  5,  4,  5, 
  14, 16, 16, 28, 18, 17, 16,  0,  0, 15, 24, 16, 18, 18, 30,  0, 
   0, 15, 16, 16, 16, 16, 15,  0,  0,  1, 14, 24, 14,  1,  0, 16, 
  14,  5,  4,  5, 14, 16,  0, 31,  2,  2,  6, 25,  0, 16, 14,  5, 
   4,  5, 14, 16,  0,  0,  0,  0,  0,  0,  0, 31, 17, 17, 17,  0, 
   0,  0,  0,  0, 17, 17, 18, 26,  0, 16, 14,  5,  4,  5, 14, 16, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,240, 16, 16, 16,224,  0,  0,  0,224, 48,224, 
   0,  0,  0, 96, 16, 16, 16, 32,  0,240,  0,  0,  0,  0,240,  0, 
   0,240, 48,192,  0,  0,240,  0,  0,  0,224, 48,224,  0,  0,  0, 
 240, 16, 16, 16,224,  0,  0,224, 16, 16, 16, 16,224,  0,  0,240, 
 112,128,  0,  0,128, 48,240,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0, 15,  1,  1,  3, 12,  0,  8,  7,  2,  2,  2, 
   7,  8,  0,  6,  8,  8,  8,  0,  0,  7, 12,  8,  8,  8,  7,  0, 
   0, 15,  0,  0,  3,  6, 15,  0,  8,  7,  2,  2,  2,  7,  8,  0, 
  15,  1,  1,  3, 12,  0,  0,  7,  8,  8,  8,  8,  7,  0,  0, 15, 
   0,  1, 14,  6,  1,  0, 15,  0,  0,  8,  0,  0,  8,  0,  0,  8, 
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

static const unsigned char displej_poziv_uart_odbijen[1024] =
{
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255, 17, 
  17, 17, 14,  0,  0,126,194,129,129,129, 67,126,  0,128,225,177, 
 137,135,129,  0,  0,  0,  0,  1, 14,112,192,112, 14,  1,  0,  0, 
   0,  0,255, 17, 17, 49,206,  0,128, 96, 44, 35, 39, 56,192,  0, 
 126,195,129,129,129, 64,  0,127,192,128,128,192,127,  0,  0,255, 
   3, 12, 48,224,255,  0,128, 96, 44, 35, 39, 56,192,  0,255, 17, 
  17, 49,206,  0,128, 96, 44, 35, 39, 56,192,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
 128,128,128,128,  0,  0,  0,128,128,128,128,128,  0,  0,  0,128, 
 128,128,128,  0,  0,  0,  0,  0,  0,  0,  0,128,  0,  0,128,128, 
 128,128,128,  0,128,128,  0,  0,  0,128,  0,  0,  0,192,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 63, 97, 
  64, 64, 64, 33, 63,  0,  0,127, 64, 64, 64, 97, 63,  0,  0,127, 
  68, 68, 68, 59,  0,  0,  0,  0,  0, 64, 64, 63,  0,  0,127, 68, 
  68, 68, 64,  0,127,  1,  6, 24,112,127,  0,  0,  0, 95,  0,  0, 
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
   0,  0,  0,  0,  0,  0,128,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
 128,  0,  0,128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
 128,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,128,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,128,128,128, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0, 32, 32, 31,  0,  0, 60, 42, 42, 46,  0,  0, 34, 
  42, 42,  0,  2, 63, 34,  0,  0, 60, 42, 42, 46,  0,  0,  0,  0, 
  63,  0,128,254,  0,  0, 62, 32, 32, 62,  0,  2, 63, 34,  0,  0, 
  62,  0,  0,  0,  0, 62,  2,  2, 62,  0,  0, 42, 42, 42, 62,  0, 
   0,  0,  0, 62,  2,  2, 62,  0,128,254,  0,  0, 60, 42, 42, 46, 
   0, 64,170,170,170, 66,  0, 42, 42, 42, 62,  0,  0,  0, 44,  4, 
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