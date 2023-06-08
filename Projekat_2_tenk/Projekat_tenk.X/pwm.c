#include "pwm.h"

void PWM_init()
{
    // Biram T2 kao tajmer za PWM module
    OC1CONbits.OCTSEL = 0;
    OC2CONbits.OCTSEL = 0;
    
    // Inicijalni duty cycle
    OC1R = 0;
    OC2R = 0;
    
    // 110 - PWM mode without fault protection input
    OC1CONbits.OCM = 0b110;
    OC2CONbits.OCM = 0b110;   
    
    // 00 - Tajmer koristi 1:1 prescale (valjda)
    T2CONbits.TCKPS = 0b00;
    //T3CONbits.TCKPS = 0b00;
       
    // duty_cycle je inicijalno 
    OC1RS = 160;
    OC2RS = 85;

    T2CONbits.TON = 1;
    //T3CONbits.TON = 1;
}

void PWM_set_duty_cycle(int new_duty_cycle1, int new_duty_cycle2)
{
   OC1RS = new_duty_cycle1;
   OC2RS = new_duty_cycle2;
}