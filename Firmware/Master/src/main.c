#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

#include "button.h"
#include "console.h"
#include "spi.h"
#include "spi-proto.h"
#include "state.h"
#include "usb-midi.h"

static volatile uint32_t system_millis;

static void clock_setup(void)
{
#if 0
    // CPU = 168 MHz, min SPI =~ 300 KHz
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
#elif 0
    // CPU = 120 MHz, min SPI =~ 250 KHz
    rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]);
#else
    // CPU = 168 MHz, min SPI =~ 160 KHz
    clock_scale_t cs = hse_8mhz_3v3[CLOCK_3V3_168MHZ];
    // cs.apb1_frequency = 12000000;
    cs.ppre2 = RCC_CFGR_PPRE_DIV_4;
    cs.apb2_frequency = 168000000 / 4;
    rcc_clock_setup_hse_3v3(&cs);
#endif

    /* clock rate / 168000 to get 1mS interrupt rate */
    systick_set_reload(168000);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();

    /* this done last */
    systick_interrupt_enable();
}

/* Called when systick fires */
void sys_tick_handler(void)
{
        system_millis++;
}

// static void print_buf(const char *label, const uint8_t *buf, size_t n)
// {
//     printf("%s %u:", label, n);
//     for (size_t i = 0; i < n; i++) {
//         uint8_t c = buf[i];
//         if (c == STX)
//             printf(" STX");
//         else if (c == ETX)
//             printf(" ETX");
//         else if (c == SYN)
//             printf(" SYN");
//         else if (' ' <= c && c < '\377')
//             // printf(" '%c'", c);
//             printf(" %c", c);
//         else
//             printf(" \\%03o", c);
//     }
//     printf("\n");
// }

#if 0
static void do_spi(void)
{
    static char seq = '0';
    #define COUNT 25
    uint8_t out[COUNT] = { STX, 'm', seq, ' ',
                           'a', 'b', 'c', 'd',
                           ' ', '1', '2', '3', 
                           '4', '5', ' ', '+',
                           '-', '*', '/', ' ',
                           'w', 'x', 'y', 'z',
                           ETX };
    uint8_t in[COUNT];
    if (++seq > '9')
        seq = '0';
    memset(in, 0, sizeof in);
    uint32_t t0 = system_millis;
    {
        spi_select_group(0);
        spi_start_transfer(5, out, in, COUNT);
        spi_finish_transfer(5);
        spi_deselect_group(0);
    }
    uint32_t t1 = system_millis;
    
    print_buf("sent    ", out, COUNT);
    print_buf("received", in, COUNT);
    printf("%ld ms\n", t1 - t0);
}
#else
static void do_spi(void)
{
    int grp;

    static spi_buf in[MODULE_COUNT];
    memset(in, 0, sizeof in);

    for (size_t i = 0; (grp = active_spi_groups(i)) != NO_GROUP; i++) {

        int bus;
        static spi_buf out[SPI_BUS_COUNT];

        spi_select_group(grp);

        // Start comm with on all buses in group
        for (size_t j = 0; (bus = active_spi_buses(grp, j)) != NO_BUS; j++) {
            size_t mod = spi_to_module(grp, bus);
            memset(in[mod], 0, sizeof in);
            int msec = system_millis;
            size_t bytes_out = assemble_outgoing_packet(out[j], msec, mod);
            spi_start_transfer(bus, out[j], in[mod], bytes_out);
            // print_buf("Send", out[j], bytes_out);
        }

        // Conclude comm with all buses
        for (size_t j = 0; (bus = active_spi_buses(grp, j)) != NO_BUS; j++) {
            spi_finish_transfer(bus);
            // size_t mod = spi_to_module(grp, bus);
            // print_buf("Receive", in[mod], sizeof in[mod]);
        }
        spi_deselect_group(grp);
    }
    // printf("\n");
}
#endif

int main()
{
    clock_setup();
    console_setup();
    console_stdio_setup();
    usb_midi_setup();
    button_setup();
    spi_setup();

    printf("Hello, World!\n");
    // printf("SYSEX address = %d\n", sc.sc_SYSEX_addr);
    // printf("sizeof synth_config = %u\n", sizeof (synth_config));
    // printf("sizeof synth_config = %u\n", sizeof (synth_state));

#ifndef NDEBUG
    verify_config();
    printf("OK\n");
#endif

    uint32_t next_time = system_millis + 20;

    while (1) {
        usb_midi_poll();
        button_poll();
        if ((int32_t)(next_time - system_millis) >= 0)
            continue;
        // printf("tick\n");
        next_time += 1;

        do_spi();

        // printf("\n");
    }
    return 0;
}
