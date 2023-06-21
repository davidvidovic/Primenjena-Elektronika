#include "motor_control.h"
#include "pwm.h"

/*
// DESNI MOTOR
// NAPRED
LATFbits.LATF1 = 1; // IN1
LATBbits.LATB11 = 0; // IN2

// NAZAD
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
    PWM_set_duty_cycle(150, 70);
    
    LATFbits.LATF1 = 1; // IN1
    LATBbits.LATB11 = 0; // IN2

    LATBbits.LATB12 = 0; // IN3
    LATFbits.LATF0 = 1; // IN4
}

void voziNazad() // NE MOZE
{
    PWM_set_duty_cycle(150, 150);
    
    LATFbits.LATF1 = 0; // IN1
    LATBbits.LATB11 = 1; // IN2

    LATBbits.LATB12 = 1; // IN3
    LATFbits.LATF0 = 0; // IN4
    
}

void skreniLevo()
{
    PWM_set_duty_cycle(200, 200);
    
    // Desni motor napred
    LATFbits.LATF1 = 1; // IN1
    LATBbits.LATB11 = 0; // IN2
    
    // Levi motor miruje
    LATBbits.LATB12 = 0;
    LATFbits.LATF0 = 0; // IN4
}

void skreniDesno()
{
    PWM_set_duty_cycle(150, 150);
    
    // Desni motor miruje
    LATFbits.LATF1 = 0; // IN1
    LATBbits.LATB11 = 0; // IN2
    
    // Levi motor napred
    LATBbits.LATB12 = 0; // IN3
    LATFbits.LATF0 = 1; // IN4
}