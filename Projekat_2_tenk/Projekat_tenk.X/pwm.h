#ifndef PWM_H
#define	PWM_H

#include <p30fxxxx.h>

void PWM_init();
void PWM_set_duty_cycle(int new_duty_cycle);

#endif	/* PWM_H */

