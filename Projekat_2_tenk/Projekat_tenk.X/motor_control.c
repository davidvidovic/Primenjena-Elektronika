#include "motor_control.h"

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