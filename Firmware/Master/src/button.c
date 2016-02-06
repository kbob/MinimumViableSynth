#include "button.h"

#include "gpio.h"
#include "midi.h"

static const gpio_pin button_gpio = {
    .gp_port = GPIOA,
    .gp_pin  = GPIO0,
    .gp_mode = GPIO_MODE_INPUT,
    .gp_pupd = GPIO_PUPD_NONE,  // board has external pulldown resistor R22.
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
    if (!button_state != !old_button_state) {
        static const uint8_t note_sequence[] = { 60, 64, 69, 62, 67 };
        static size_t i, j;
        uint8_t note = note_sequence[i];
        if (j == 5)
            note -= 36;
        if (button_state)
            MIDI_send_note_on(MIDI_default_channel, note, 96);
        else {
            MIDI_send_note_off(MIDI_default_channel, note, 96);
            i = (i + 1) % sizeof note_sequence;
            j = (j + 1) % 6;
        }
    }
}
