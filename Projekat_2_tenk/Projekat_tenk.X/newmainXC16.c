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

#include "uart.h"
#include "ADC.h"
#include "timer.h"
#include "pins.h"
#include "hc-sr04.h"


_FOSC(CSW_FSCM_OFF & XT_PLL4); // takt 10MHz
_FWDT(WDT_OFF); // Watch Dog Timer - OFF
//_FGS(CODE_PROT_OFF);

// -----------------------------------
// GLOBALNE PROMENJIVE

char tempRX_BLE;
char tempRX_DEBUG;
int brojac_ms;
int brojac_us;
int vrednost_analogni_senzor;
int vrednost_Rsens1;
int vrednost_Rsens2;
unsigned char overflow_flag_desno;
unsigned char overflow_flag_levo;
int distancaDesno;
int distancaLevo;

char word_START[10] = "";
int indeks = 0;


// -----------------------------------
// DELAY FUNKCIJE


void Delay_us(int pauza)
{
    brojac_us = 0;
    T5CONbits.TON = 1; // T5 on
    
    while(brojac_us < pauza);
    
    T5CONbits.TON = 0; // T5 off
}

/*
void Delay_ms(int pauza)
{
    brojac_ms = 0;
    T5CONbits.TON = 1; // T5 on
    
    while(brojac_ms < pauza);
    
    T5CONbits.TON = 0; // T5 off
}
*/

// -----------------------------------
// PREKIDNE RUTINE


// Prekidna rutina za UART1 - DEBUG svrhe
void __attribute__((__interrupt__, no_auto_psv)) _U1RXInterrupt(void) 
{
    tempRX_DEBUG = U1RXREG;
	
    // KOD
  
    IFS0bits.U1RXIF = 0;
} 


// Prekidna rutina za UART2 - BLE komunikacija
void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void) 
{
    IFS1bits.U2RXIF = 0;
    tempRX_BLE = U2RXREG;
    
    // KOD
    if(tempRX_BLE != 0)
    {
        word_START[indeks] = tempRX_BLE;
        tempRX_BLE = 0;

        if(indeks < 6)
           indeks++;
        else indeks = 0;
    }
}  


// Prekidna rutina za TIMER1 - desni HCSR04 echo
void __attribute__ ((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
	IFS0bits.T1IF = 0;
    overflow_flag_desno = 1;
    PORTDbits.RD8 = 0;  
}

// Prekidna rutina za TIMER2 koji se koristi za PWM1
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void) // pwm
{
    TMR2 = 0;
    IFS0bits.T2IF = 0;
}


// Prekidna rutina za TIMER3 koji se koristi za PWM2
void __attribute__ ((__interrupt__, no_auto_psv)) _T3Interrupt(void)
{
    TMR3 = 0;
    IFS0bits.T3IF = 0;
} 


// Prekidna rutina za TIMER4
void __attribute__ ((__interrupt__, no_auto_psv)) _T4Interrupt(void)
{
    IFS1bits.T4IF = 0;
    overflow_flag_levo = 1;
    PORTDbits.RD9 = 0;
} 


// Prekidna rutina za TIMER5 koji broji mikrosekunde (us)
void __attribute__ ((__interrupt__, no_auto_psv)) _T5Interrupt(void)
{
	TMR5 = 0;   
	brojac_us++;
	IFS1bits.T5IF = 0;    
} 


// Prekidna rutina za AD konverziju
void __attribute__((__interrupt__, no_auto_psv)) _ADCInterrupt(void) 
{
    vrednost_analogni_senzor = ADCBUF0;
    vrednost_Rsens1 = ADCBUF1;
    vrednost_Rsens2 = ADCBUF2;
    
    // Gasimo AD konverziju
    ADCON1bits.ADON = 0;
    IFS0bits.ADIF = 0;
} 


/*
 * IDEJA RACUNANJA DISTANCE SA HC-SR04:
 * Pin change interrupt (INT1 ili INT2) pokrenu tajmer kada se desi da je ECHO = 1
 * Za jedan HCSR04 koristim T3 a za drugi T4
 * Oba su podesena da se prekidna rutina obradjuje na 5ms (sa skalerom 50000 u timer.c fajlu)
 * Brzina zvuka je 343m/s, sto znaci da on za 1ms prijedje 34cm, medjutim ove senzori rade tako sto posalju signal
 * I on im se vrati (dupla putanja), tako da ako bi imali tajmer koji broji svaku 1ms,
 * Mogli bi maks izmjeriti 17cm bez da kompikujemo kod
 * Ideja je da kada se upali tajmer, njegov TMR registar se uvecava
 * Oba tajmera su 16-bita tako da je maks vrijednost njihovih TMR registara 2^16 (65536)
 * Kada TMR registar dostigne tu vrijednost, tada se aktivira i obradjuje prekidna rutina
 * 
 * Kada se tajmer pusti njegov TMR registar se uvecava sve dok se ne desi nesto od sledeceg:
 * 1. Pin Change Interrupt na echo pinu (spustio se na 0 - signal se vratio) - u tom slucaju citamo vrijednost
 *    iz TMR registra, dijelimo ju sa faktorom 0.017 (po formuli sa brzinom zvuka, ne pitaj)
 * 2. Echo pin nakon 5ms ostane na 1 a TMR registar odboji do 2^16 i obradi se prekidna rutina tajmera,
 *    u tom slucaju rucno spustamo ECHO pin na 0 i setujemo overflow flag da bi znali sta je uzrokovalo prekid mjerenja
 *    I za racunanje distance uzimamo max vrijednost TMR-a, tako da smo ovim ogranicili da je maksimalna
 *    udaljenost koju HCSR04 moze izmjeriti 112cm (za potrebe projekta nam ni ne treba vise)
 */

// Prekidna rutina za HC-SR04 1 (Levi)
void __attribute__ ((__interrupt__, no_auto_psv)) _INT2Interrupt(void)
{   
    // Ako je interrupt desio jer je ECHO pin postao 1
    // Treba da zapocnemo T4 i merenje vremena   
    if(PORTDbits.RD9 == 1)
    {
        // Neka je INT2 senzitivan na opadajucu ivicu
        INTCON2bits.INT2EP = 1;
        
        // Pocinjemo meriti vreme koliko je ECHO pin na logickoj jedinici
        TMR4 = 0;
        
        // Spustam flag T4 interrupta
        IFS1bits.T4IF = 0;

        // T4 ON
        T4CONbits.TON = 1; 
    }
    
    // Ako je interrupt desio jer je ECHO pin postao 0
    // Treba da stopiramo T4 i merenje vremena
    // I izracunamo distancu
    else
    {
        // Gasim T3
        T4CONbits.TON = 0;
        
        if(overflow_flag_levo)
        {
            distancaLevo = 65536 * 0.01715 / 10;
        }
        else
        {
            distancaLevo = TMR4 * 0.01715 / 10;
        }
        
        overflow_flag_levo = 0;
        
        // Neka je INT2 senzitivan na rastucu ivicu
        INTCON2bits.INT2EP = 0;
    }
    
    IFS1bits.INT2IF = 0;
}



// Prekidna rutina za HC-SR04 2 (Desni)
void __attribute__ ((__interrupt__, no_auto_psv)) _INT1Interrupt(void)
{   
    // Ako je interrupt desio jer je ECHO pin postao 1
    // Treba da zapocnemo T1 i merenje vremena   
    if(PORTDbits.RD8 == 1)
    {
        // Neka je INT1 senzitivan na opadajucu ivicu
        INTCON2bits.INT1EP = 1;
        
        // Pocinjemo meriti vreme koliko je ECHO pin na logickoj jedinici
        TMR1 = 0;
        
        // Spustam flag T1 interrupta
        IFS0bits.T1IF = 0;

        // T1 ON
        T1CONbits.TON = 1; 
    }
    
    // Ako je interrupt desio jer je ECHO pin postao 0
    // Treba da stopiramo T1 i merenje vremena
    // I izracunamo distancu
    else
    {
        // Gasim T1
        T1CONbits.TON = 0;
        
        if(overflow_flag_desno)
        {
            distancaDesno = 65536 * 0.01715 / 10;
        }
        else
        {
            distancaDesno = TMR1 * 0.01715 / 10;
        }
        
        overflow_flag_desno = 0;
        
        // Neka je INT1 senzitivan na rastucu ivicu
        INTCON2bits.INT1EP = 0;
    }
    
    IFS1bits.INT1IF = 0;
}


void meriDesno()
{
    // Dajem trigger pinu logicku jedinicu u trajanju 10us
    LATDbits.LATD2 = 1;
    Delay_us(10);
    LATDbits.LATD2 = 0;
}

void meriLevo()
{
    // Dajem trigger pinu logicku jedinicu u trajanju 10us
    LATDbits.LATD3 = 1;
    Delay_us(10);
    LATDbits.LATD3 = 0;
}

// Oko 1100 vrednosti analognog senzora je prepreka na 10cm udaljenosti
void meriIspred()
{
    ADCON1bits.ADON = 1;
    Delay_us(1000);
    ADCON1bits.ADON = 0;
}


void PWM()
{
    // Biram T2 kao tajmer za PWM1 modul
    OC1CONbits.OCTSEL = 0;
    // Biram T3 kao tajmer za PWM1 modul
    OC2CONbits.OCTSEL = 1;
    
    // Inicijalni duty cycle
    OC1R = 0;
    OC2R = 0;
    
    // 110 - PWM mode without fault protection input
    OC1CONbits.OCM = 0b110;
    OC2CONbits.OCM = 0b110;   
    
    // 00 - Tajmer koristi 1:1 prescale (valjda)
    T2CONbits.TCKPS = 0b00;
    T3CONbits.TCKPS = 0b00;
       
    OC1RS = 350;
    OC2RS = 350;

    T2CONbits.TON = 1;
    T3CONbits.TON = 1;
}


void stani()
{
    LATBbits.LATB11 = 0;
    LATFbits.LATF1 = 0;

    LATBbits.LATB12 = 0;
    LATFbits.LATF0 = 0;
}

void voziNapred()
{
    LATBbits.LATB11 = 1;
    LATFbits.LATF1 = 0;

    LATBbits.LATB12 = 1;
    LATFbits.LATF0 = 0;
}

void voziNazad()
{
    LATBbits.LATB11 = 0;
    LATFbits.LATF1 = 1;

    LATBbits.LATB12 = 0;
    LATFbits.LATF0 = 1;
}

void skreniLevo()
{
      
}

void skreniDesno()
{
    
}



int main(void) {
    // Inicijalizacija modula
    InitUART1();
    InitUART2();
    Init_T1();
    Init_T2();
    Init_T3();
    Init_T4();
    Init_T5();
    InitPins();
    InitInterrupts();
    ADCinit();
    
    ADCON1bits.ADON = 0;
    
    overflow_flag_desno = 0;
    overflow_flag_levo = 0;
    
    
    print_BLE("Inicijalizacija zavrsena!");
    
    indeks = 0;
    
    
    stani();
    PWM();    
    
     
    // Glavna - super petlja
    while(1)
    {
        
        while(word_START[0] != 'S' &&
              word_START[1] != 'T' &&
              word_START[2] != 'A' &&
              word_START[3] != 'R' &&
              word_START[4] != 'T' 
        );
        
        word_START[0] = 0;
        word_START[1] = 0;
        word_START[2] = 0;
        word_START[3] = 0;
        indeks = 0;
        
        print_BLE("Pokretanje!");
        
        
        
        while(word_START[0] != 'S' &&
              word_START[1] != 'T' &&
              word_START[2] != 'O' &&
              word_START[3] != 'P' 
              )
        {
            meriLevo();
            meriIspred();
            meriDesno();
        
        
            if(distancaLevo > 10 + 5) // +5 jer je senzor 5cm udaljen od ivice tocka
            {
                //skreniLevo();
                print_BLE("Skrecem levo\n");
            }
            else if(vrednost_analogni_senzor < 1200)
            {
                //voziNapred();
                print_BLE("Napred\n");
            }
            else if(distancaDesno > 10 + 8) // 8cm udaljen od desne ivice tenka
            {
                //skreniDesno();
                print_BLE("Skrecem desno\n");
            }
            else
            {
                //voziNazad();
                print_BLE("Vozim unazad\n");
            }
        }
        
        print_BLE("Zaustavljanje.");
        
        indeks = 0;
       
        
    }
    return 0;
} 
