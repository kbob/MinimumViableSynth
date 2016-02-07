#include "gpio.h"

#include <assert.h>
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>

#define GPIO_PORT_COUNT 11

static bool gpio_clock_enabled[GPIO_PORT_COUNT];

void gpio_init_pin(const gpio_pin *pin)
{
    uint32_t index =
        ((uint32_t)pin->gp_port - (uint32_t)PERIPH_BASE_AHB1) >> 10;
    assert(index < GPIO_PORT_COUNT);

    if (!gpio_clock_enabled[index]) {
        rcc_periph_clock_enable((0x30 << 5) | index);
        gpio_clock_enabled[index] = true;
    }

    gpio_mode_setup(pin->gp_port,
                    pin->gp_mode,
                    pin->gp_pupd,
                    pin->gp_pin);

    if (pin->gp_mode == GPIO_MODE_OUTPUT || pin->gp_mode == GPIO_MODE_AF)
        gpio_set_output_options(pin->gp_port,
                                pin->gp_otype,
                                pin->gp_ospeed,
                                pin->gp_pin);

    if (pin->gp_mode == GPIO_MODE_AF)
        gpio_set_af(pin->gp_port,
                    pin->gp_af,
                    pin->gp_pin);
}

void gpio_init_pins(const gpio_pin *pins, size_t count)
{
    for (size_t i = 0; i < count; i++)
        gpio_init_pin(&pins[i]);
}
