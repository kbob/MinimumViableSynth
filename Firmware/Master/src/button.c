#include "button.h"

#include "gpio.h"
#include "usb-midi.h"

static const gpio_pin button_gpio = {
    .gp_port = GPIOA,
    .gp_pin  = GPIO0,
    .gp_mode = GPIO_MODE_INPUT,
    .gp_pupd = GPIO_PUPD_NONE,
};

void button_setup(void)
{
    /* Button pin */
    gpio_init_pin(&button_gpio);
}

void button_poll(void)
{
    static uint32_t button_state = 0;

    /* This is a simple shift based debounce. It's simplistic because
     * although this implements debounce adequately it does not have any
     * noise suppression. It is also very wide (32-bits) because it can
     * be polled in a very tight loop (no debounce timer).
     */
    uint32_t old_button_state = button_state;
    button_state = (button_state << 1) | (GPIOA_IDR & 1);
    if ((0 == button_state) != (0 == old_button_state))
        usb_midi_send_note((bool)button_state);
}

