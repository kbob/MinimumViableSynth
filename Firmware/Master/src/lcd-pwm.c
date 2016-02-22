#include "lcd-pwm.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "gpio.h"

// PWM brightness pin
//   Pin PF6
//   Timer TIM10, channel 1
//   Alt function AF3

// Display On/Off pin
//   Pin PE4


#define PWM_PORT  GPIOF
#define PWM_PIN   GPIO6
#define DISP_PORT GPIOE
#define DISP_PIN  GPIO4

static const gpio_pin lcd_pwm_pins[] = {
    {
        .gp_port  = PWM_PORT,
        .gp_pin   = PWM_PIN,
        .gp_mode  = GPIO_MODE_AF,
        .gp_af    = GPIO_AF3,
    },
    {
        .gp_port  = DISP_PORT,
        .gp_pin   = DISP_PIN,
        .gp_mode  = GPIO_MODE_OUTPUT,
        .gp_level = 0,
    },
};
static const size_t lcd_pwm_pin_count = (&lcd_pwm_pins)[1] - lcd_pwm_pins;

void lcd_pwm_setup(void)
{
    // enable clock
    rcc_periph_clock_enable(RCC_TIM10);

    // init GPIOs
    gpio_init_pins(lcd_pwm_pins, lcd_pwm_pin_count);

    // configure timer
    timer_reset(TIM10);
    // timer_set_mode(TIM10, TIM_CR1_CKD_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_oc_mode(TIM10, TIM_OC1, TIM_OCM_PWM1);
    timer_enable_oc_output(TIM10, TIM_OC1);
    timer_set_oc_value(TIM10, TIM_OC1, 0); // start out dark.
    timer_set_period(TIM10, 65535);
    timer_enable_counter(TIM10);
}

void lcd_pwm_set_brightness(uint16_t brightness)
{
    timer_set_oc_value(TIM10, TIM_OC1, brightness);
    if (brightness)
        gpio_set(DISP_PORT, DISP_PIN);
    else
        gpio_clear(DISP_PORT, DISP_PIN);
}
