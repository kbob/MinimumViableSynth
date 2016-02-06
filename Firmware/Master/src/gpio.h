#ifndef GPIO_included
#define GPIO_included

#include <stddef.h>

#include <libopencm3/stm32/gpio.h>

typedef struct gpio_pin {
    uint32_t gp_port;
    uint16_t gp_pin;
    uint8_t  gp_mode;
    uint8_t  gp_pupd;
    uint8_t  gp_af;
    uint8_t  gp_ospeed;
    uint8_t  gp_otype;
} gpio_pin;

extern void gpio_init_pin(const gpio_pin *);

extern void gpio_init_pins(const gpio_pin *, size_t count);

#endif /* !GPIO_included */
