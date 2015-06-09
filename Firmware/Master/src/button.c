#include "button.h"

#include <stdio.h>

#include <libopencm3/stm32/gpio.h>

#include "usb-midi.h"

void button_setup(void)
{
    /* Button pin */
    gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
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
    // printf("button_state = %08lx\n", button_state);
    if ((0 == button_state) != (0 == old_button_state))
        usb_midi_send_note((bool)button_state);
}

