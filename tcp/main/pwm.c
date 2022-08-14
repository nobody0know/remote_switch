#include "driver/mcpwm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH_US (500) // Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH_US (2500) // Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE        (180)   // Maximum angle in degree upto which servo can rotate

#define SERVO_PULSE_GPIO_LEFT        (18)   // GPIO connects to the PWM signal line
#define SERVO_PULSE_GPIO_RIGHT       (19)   //两个舵机使用的引脚口

#define left_turn_on 11
#define left_turn_off 12
#define right_turn_on 21
#define right_turn_off 22
#define all_on 31
#define all_off 32
static inline uint32_t convert_servo_angle_to_duty_us(int angle)
{
    return (angle + SERVO_MAX_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (2 * SERVO_MAX_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

void servo_control_init(void)
{
    mcpwm_pin_config_t servo_pin_init = {
        .mcpwm0a_out_num = 18,
        .mcpwm0b_out_num = 19,
    };
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PULSE_GPIO_LEFT); // To drive a RC servo, one MCPWM generator is enough
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, SERVO_PULSE_GPIO_RIGHT);
    mcpwm_set_pin(MCPWM_UNIT_0,&servo_pin_init);
    mcpwm_config_t pwm_config = {
        .frequency = 50, // frequency = 50Hz, i.e. for every servo motor time period should be 20ms
        .cmpr_a = 0,     // duty cycle of PWMxA = 0
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

}

/*
mode
11 左边开
12 左边关
21 右边开
22 右边关
31 全开
32 全关
*/
void servo_control(uint16_t mode)
{
    switch (mode)
    {
    case left_turn_on:
        printf("turn on the left switch\n");
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(30)));//左后打下去
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(-60)));//平齐
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    case left_turn_off:
        printf("turn off the left switch\n");
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(-160)));//左后打下去
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(-60)));//平齐
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    case right_turn_on:
        printf("turn on the right switch\n");
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(-160)));//左后打下去
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(-60)));//平齐
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    case right_turn_off:
        printf("turn off the right switch\n");
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(30)));//左后打下去
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(-60)));//平齐
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    case all_on:
        printf("all on\n");
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(30)));//左后打下去
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(-160)));//左后打下去
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(-60)));//平齐
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(-60)));//平齐
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    case all_off:
        printf("all off\n");
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(-160)));//左后打下去
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(30)));//左后打下去
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, convert_servo_angle_to_duty_us(-60)));//平齐
        ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, convert_servo_angle_to_duty_us(-60)));//平齐
        vTaskDelay(pdMS_TO_TICKS(500));
        break;
    default:
        break;
    }

}