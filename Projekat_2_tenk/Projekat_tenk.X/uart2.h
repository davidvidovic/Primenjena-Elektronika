#include <p30fxxxx.h>
//#define UART2_H

// UART2 se u ovom projektu koristi za Debugging

// Funkcija za inicijalizaciju UART1
void initUART2(void);

// Funkcija za ispis karaktera preko UART1
void WriteUART2(unsigned int data);

// Funkcija za slanje stringa preko UART1
void print_DEBUG(register const char *str);

// Funkcija za slanje 4-cifrenog broja preko UART1
void WriteUART2dec2string(unsigned int data);




// ********************************************



// REALIZACIJA PREKIDNE RUTINE MORA BITI U MAIN FUNKCIJI
// I IZGLEDA OVAKO

/* 
void __attribute__((__interrupt__)) _U2RXInterrupt(void) //interrupt za UART
{
    IFS0bits.U2RXIF = 0;
    tempRX_DEBUG = 0;
    tempRX_DEBUG = U2RXREG;    
            
} 
*/

// GDEJ JE unsigned char tempRX_DEBUG GLOBALNA PROMENJIVA