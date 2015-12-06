#include <stdio.h>
#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

#include "button.h"
#include "console.h"
#include "spi.h"
#include "spi-proto.h"
#include "state.h"
#include "systick.h"
#include "usb-midi.h"

#define SPI_POLL_INTERVAL_MSEC 100

static spi_buf outgoing_packets[MODULE_COUNT];
static spi_buf incoming_packets[MODULE_COUNT];

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
}

#if 0

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

static volatile uint32_t completion_count;

static void SPI_completion_handler()
{
    completion_count++;
}

static void do_spi(void)
{
    int grp;

    memset(incoming_packets, 0, sizeof incoming_packets);

    for (int i = 0; (grp = active_spi_groups(i)) != NO_GROUP; i++) {

        int bus;

        spi_select_group(grp);

        // Start comm on all buses in group
        for (size_t j = 0; (bus = active_spi_buses(grp, j)) != NO_BUS; j++) {
            size_t mod = spi_to_module(grp, bus);
            if (mod >= MODULE_COUNT) {
                printf("grp %d bus %d => mod %d\n", grp, bus, mod);
                continue;
            }
            uint32_t msec = system_millis;
            size_t bytes_out =
                assemble_outgoing_packet(outgoing_packets[mod], msec, mod);
            spi_start_transfer(bus,
                               outgoing_packets[mod],
                               incoming_packets[mod],
                               bytes_out);
            // print_buf("Send", outgoing_packets[mod], bytes_out);
        }

        // Conclude comm with all buses
        for (size_t j = 0; (bus = active_spi_buses(grp, j)) != NO_BUS; j++) {
            spi_finish_transfer(bus);
        }
        spi_deselect_group(grp);
        printf("completion_count = %lu\n", completion_count);
        
        // Parse all the received packets
        for (size_t j = 0; (bus = active_spi_buses(grp, j)) != NO_BUS; j++) {
            size_t mod_index = spi_to_module(grp, bus);
            const module_config *mod = &sc.sc_modules[mod_index];
            slave_state state;
            bool ok = parse_incoming_packet(incoming_packets[mod_index],
                                            sizeof incoming_packets[mod_index],
                                            mod_index,
                                            &state);
            if (ok) {
                if (!state.ss_is_valid) {
                    printf("%s: not valid\n", mod->mc_name);
                } else {
                    if (state.ss_buttons)
                        printf("%s: buttons = %#x\n",
                               mod->mc_name, state.ss_buttons);
                    if (state.ss_analog_mask)
                        for (int j = 0; j < MAX_KNOBS; j++)
                            if (state.ss_analog_mask & (1 << j))
                                printf("%s: analog[%d] = %u\n",
                                       mod->mc_name,
                                       j, state.ss_analog_values[j]);
                }
            } else {
                for (size_t j = 0; j < sizeof (spi_buf); j++)
                    printf("%#4o ", incoming_packets[mod_index][j]);
                printf("\n");
                printf("incoming packet failed\n\n");
            }
        }
    }
}

#endif

int main()
{
    clock_setup();
    systick_setup(168000000);
    console_setup();
    console_stdio_setup();
    usb_midi_setup();
    button_setup();
    spi_setup();
    spi_register_completion_handler(SPI_completion_handler);

    printf("Hello, World!\n");

#ifndef NDEBUG
    verify_config();
    printf("OK\n");
#endif

    uint32_t next_time = system_millis + 1;

    while (1) {
        usb_midi_poll();
        button_poll();

        if ((int32_t)(next_time - system_millis) >= 0)
            continue;
        next_time += SPI_POLL_INTERVAL_MSEC;

        do_spi();
    }
    return 0;
}
