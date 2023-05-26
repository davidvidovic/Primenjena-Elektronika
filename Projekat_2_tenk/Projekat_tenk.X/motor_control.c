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
    LATFbits.LATF1 = 1;

    LATBbits.LATB12 = 1;
    LATFbits.LATF0 = 1;
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
    // DESNI MOTOR
    // NAPRED
    LATFbits.LATF1 = 1; // IN1
    LATBbits.LATB11 = 0; // IN2
    
    // NAPRED
    LATFbits.LATF1 = 0; // IN1
    LATBbits.LATB11 = 1; // IN2
    
    // LEVI MOTOR
    // NAPRED
    LATBbits.LATB12 = 0; // IN3
    LATFbits.LATF0 = 1; // IN4
    
    // NAZAD
    LATBbits.LATB12 = 1; // IN3
    LATFbits.LATF0 = 0; // IN4
}

void skreniDesno()
{
    
}