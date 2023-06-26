#ifndef PWM_H
#define	PWM_H

#include <p30fxxxx.h>

void PWM_init(void);
void PWM_set_duty_cycle(int new_duty_cycle1, int new_duty_cycle2);

#endif	/* PWM_H */

