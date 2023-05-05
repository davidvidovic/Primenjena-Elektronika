#include "hc-sr04.h"


void InitInterrupts()
{
    // INT1 ON
    IEC1bits.INT1IE = 1;    
    // Spustam INT1 flag
    IFS1bits.INT1IF = 0;
    // INT1 senzitivan na rastucu ivicu
    INTCON2bits.INT1EP = 0;
      
    // INT2 ON
    IEC1bits.INT2IE = 1; 
    // Spustam INT2 flag
    IFS1bits.INT2IF = 0;
    // INT2 senzitivan na rastucu ivicu
    INTCON2bits.INT2EP = 0;
}

