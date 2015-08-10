#ifndef GPIO_included
#define GPIO_included

#include <libopencm3/stm32/gpio.h>

typedef struct gpio_pin {
    uint32_t gp_port;
    uint16_t gp_pin;
    uint8_t  gp_mode;
    uint8_t  gp_af;
    uint8_t  gp_pupd;
} gpio_pin;

extern void gpio_init_pin(const gpio_pin *);

#endif /* !GPIO_included */
