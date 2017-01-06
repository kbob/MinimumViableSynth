#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>

#include "button.h"
#include "console.h"
#include "delay.h"
#include "lcd-dma.h"
#include "lcd-pwm.h"
#include "midi.h"
#include "sdram.h"
#include "spi.h"
#include "spi-proto.h"
#include "spi-responder.h"
#include "state.h"
#include "systick.h"
#include "usb-midi.h"

#define REPORT_INTERVAL_MSEC 10000

#define CPU_FREQ 168000000
// #define CPU_FREQ 120000000
// #define CPU_FREQ 48000000

static void clock_setup(void)
{
    if (CPU_FREQ == 168000000) {
        // CPU = 168 MHz, min SPI =~ 328 KHz
        rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
    } else if (CPU_FREQ == 120000000) {
        // CPU = 120 MHz, min SPI =~ 234 KHz
        rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_120MHZ]);
    } else if (CPU_FREQ == 48000000) {
        // CPU = 48 MHz, min SPI =~ 94 KHz
        rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_48MHZ]);
    }
}

static void adjust_LCD_brightness(uint32_t now)
{
    static bool done;
    if (done)
        return;
    static uint32_t t0;
    if (!t0) {
        t0 = now + 1;
        return;
    }
    uint32_t b0 = (now - t0);
    uint32_t b1 = b0 * (b0 + 1) >> 5;
    if (b1 > 65535) {
        b1 = 65535;
        done = true;
    }
    lcd_pwm_set_brightness(b1);
}

int main()
{
    clock_setup();
    systick_setup(CPU_FREQ);
    lcd_pwm_setup(); // Do this early to keep the uninitialized screen dark.
    console_setup();
    console_stdio_setup();
    MIDI_setup();
    usb_midi_setup();
    button_setup();
    spi_setup();
    SPI_proto_setup();
    SPI_responder_setup();
    sdram_setup();
    lcd_dma_setup();
    init_delay(CPU_FREQ);

    printf("Minimum Viable Firmware\n");

#ifndef NDEBUG
    verify_config();
    printf("OK\n");
#endif

    uint32_t next_time = REPORT_INTERVAL_MSEC;

    while (1) {
        adjust_LCD_brightness(system_millis);
        usb_midi_poll();
        button_poll();

        if ((int32_t)(next_time - system_millis) > 0)
            continue;
        next_time += REPORT_INTERVAL_MSEC;

        SPI_proto_report_and_clear_stats();
        // USB_midi_report_and_clear_stats();
    }
    return 0;
}
