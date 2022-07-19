#include "driver/mcpwm.h"
// You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH_US (500) // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US (2500) // Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE        (180)   // Maximum angle in degree upto which servo can rotate

#define SERVO_PULSE_GPIO_LEFT        (18)   // GPIO connects to the PWM signal line
#define SERVO_PULSE_GPIO_RIGHT       (19)

static inline uint32_t convert_servo_angle_to_duty_us(int angle)
{
    return (angle + SERVO_MAX_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (2 * SERVO_MAX_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

void servo_control_init(void)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PULSE_GPIO_LEFT); // To drive a RC servo, one MCPWM generator is enough
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, SERVO_PULSE_GPIO_RIGHT);
    mcpwm_config_t pwm_config = {
        .frequency = 50, // frequency = 50Hz, i.e. for every servo motor time period should be 20ms
        .cmpr_a = 0,     // duty cycle of PWMxA = 0
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

}

void servo_control_left(void)
{

}