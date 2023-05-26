#include "motor_control.h"

/*
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
 */

void stani()
{
    LATBbits.LATB11 = 0;
    LATFbits.LATF1 = 0;

    LATBbits.LATB12 = 0;
    LATFbits.LATF0 = 0;
}

void voziNapred()
{
    LATFbits.LATF1 = 1; // IN1
    LATBbits.LATB11 = 0; // IN2

    LATBbits.LATB12 = 0; // IN3
    LATFbits.LATF0 = 1; // IN4
}

void voziNazad()
{
    LATFbits.LATF1 = 0; // IN1
    LATBbits.LATB11 = 1; // IN2

    LATBbits.LATB12 = 1; // IN3
    LATFbits.LATF0 = 0; // IN4
}

void skreniLevo()
{
    // Desni motor napred
    LATFbits.LATF1 = 1; // IN1
    LATBbits.LATB11 = 0; // IN2
    
    // Levi motor nazad
    LATBbits.LATB12 = 1; // IN3
    LATFbits.LATF0 = 0; // IN4
}

void skreniDesno()
{
    // Desni motor nazad
    LATFbits.LATF1 = 0; // IN1
    LATBbits.LATB11 = 1; // IN2
    
    // Levi motor napred
    LATBbits.LATB12 = 0; // IN3
    LATFbits.LATF0 = 1; // IN4
}