#include <p30fxxxx.h>
//#define UART1_H

// UART1 se u ovom projektu koristi za Debugging

// Funkcija za inicijalizaciju UART1
void initUART1(void);

// Funkcija za ispis karaktera preko UART1
void WriteUART1(unsigned int data);

// Funkcija za slanje stringa preko UART1
void print_DEBUG(register const char *str);

// Funkcija za slanje 4-cifrenog broja preko UART1
void WriteUART1dec2string(unsigned int data);




// ********************************************



// REALIZACIJA PREKIDNE RUTINE MORA BITI U MAIN FUNKCIJI
// I IZGLEDA OVAKO

/* 
void __attribute__((__interrupt__)) _U1RXInterrupt(void) //interrupt za UART
{
    IFS0bits.U1RXIF = 0;
    tempRX = 0;
    tempRX = U1RXREG;    
            
} 
*/

// GDEJ JE unsigned char tempRX GLOBALNA PROMENJIVA