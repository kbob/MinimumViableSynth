#include "spi-proto.h"

#include <assert.h>
#include <string.h>

#include "anim.h"
#include "config.h"
#include "spi.h"
#include "state.h"

// MOSI message format
//
//   STX : 1 byte
//   config 0 : 1 byte
//     bit 0:      c1 = nonzero if choice one lit
//     bit 1:      c2 = nonzero if choice two lit
//     bit 2:      c3 = nonzero if choice three lit
//     bit 3:      c4 = nonzero if choice four lit
//     bit 4:      c5 = nonzero if choice five lit
//     bit 5:      c6 = nonzero if choice six lit
//     bit 6:       (not used)
//     bit 7:      ap = nonzero if assign LED lit
//   config 1: 1 byte
//     bits 0-2:   np = number of pixels
//     bits 3-5:   na = number of analog inputs
//     bits 6-7:   unused
//   choice LED pwm: 1 byte per lit choice
//   assign LED pwm: 1 byte
//   pixels: 3 * np bytes
//     1st: title
//     2nd - np'th: knobs
//     red, green, blue order
//   checksum : 2 bytes
//   ETX : 1 byte

// active_modules is a hack to allow turning the synth on one module at a time.
static size_t active_modules[] = {
    M_LFO1,
    M_OSC1,
};
static size_t active_module_count = (&active_modules)[1] - active_modules;

static void module_to_spi(size_t   module_index,
                          uint8_t *spi_group_out,
                          uint8_t *spi_bus_out)
{
    assert(module_index < MODULE_COUNT);
    const module_config *mcp = &sc.sc_modules[module_index];
    if (spi_group_out)
        *spi_group_out = mcp->mc_SPI_group;
    if (spi_bus_out)
        *spi_bus_out = mcp->mc_SPI_bus;
}

size_t spi_to_module(uint8_t spi_group, uint8_t spi_bus)
{
    static size_t spi2mod[SPI_GROUP_COUNT][SPI_BUS_RANGE];
    static bool initialized;

    if (!initialized) {
        for (size_t i = 0; i < MODULE_COUNT; i++) {
            uint8_t grp, bus;
            module_to_spi(i, &grp, &bus);
            spi2mod[grp][bus] = i;
        }
        initialized = true;
    }
    return spi2mod[spi_group][spi_bus];
}

int active_spi_groups(uint8_t index)
{
    static int active_groups[SPI_GROUP_COUNT];
    static int active_group_count;

    if (!active_group_count) {
        bool active[SPI_GROUP_COUNT];
        memset(active, 0, sizeof active);
        for (size_t i = 0; i < active_module_count; i++) {
            uint8_t grp, bus;
            module_to_spi(active_modules[i], &grp, &bus);
            active[grp] = true;
        }
        for (size_t g = 0; g < SPI_GROUP_COUNT; g++)
            if (active[g])
                active_groups[active_group_count++] = g;
    }

    if (index < active_group_count)
        return active_groups[index];
    else
        return NO_GROUP;
}

int active_spi_buses(uint8_t group, uint8_t index)
{
    static int active_buses[SPI_GROUP_COUNT][SPI_BUS_COUNT];
    static int active_bus_count[SPI_GROUP_COUNT];
    static bool initialized;

    if (!initialized) {
        bool active[SPI_GROUP_COUNT][SPI_BUS_RANGE];
        memset(active, 0, sizeof active);
        for (size_t i = 0; i < active_module_count; i++) {
            uint8_t grp, bus;
            module_to_spi(active_modules[i], &grp, &bus);
            active[grp][bus] = true;
        }
        for (size_t g = 0; g < SPI_GROUP_COUNT; g++)
            for (size_t b = 0; b < SPI_BUS_RANGE; b++)
                if (active[g][b]) {
                    assert(active_bus_count[g] < SPI_BUS_COUNT);
                    active_buses[g][active_bus_count[g]++] = b;
                }
        initialized = true;
    }

    if (group >= SPI_GROUP_COUNT)
        return NO_GROUP;
    if (index < active_bus_count[group])
        return active_buses[group][index];
    else
        return NO_BUS;
}

static uint16_t fletcher16(const uint8_t *p, size_t count)
{
    uint16_t sum = 0;
    uint16_t sum2 = 0;
    for (size_t i = 0; i < count; i++) {
        sum += p[i];
        if (sum >= 255)
            sum -= 255;
        sum2 += sum;
        if (sum2 >= 255)
            sum2 -= 255;
    }
    return sum2 << 8 | sum;
}

// Need:
//   choice value, number of choices
//   assign value, whether assign exists
//   pixel count
//   pixel values
//     module: ms_LED()
//     knobs:  ks_LED()
//   knob count
//   choice PWM value
//   assign PWM value
//
// Define some convenience routines to calculate the LED values.

size_t assemble_outgoing_packet(spi_buf packet,
                                uint32_t msec,
                                size_t module_index)
{
    const module_config *mcp = &sc.sc_modules[module_index];
    const module_state *msp = &ss.ss_modules[module_index];

    // These variables are copied into the packet.
    uint8_t cfg0 = 0;
    uint8_t cfg1 = 0;
    uint8_t pwm[MAX_CHOICES + MAX_ASSIGNS];
    size_t pwm_count = 0;
    uint8_t pixels[MAX_PIXELS][3];
    size_t pixel_count = 0;

    // Collect the info.
    if (mcp->mc_has_choice) {
        cfg0 |= 1 << msp->ms_choice.cs_value;
        pwm[pwm_count++] = anim_choice_brightness(msec, module_index);
    }

    if (mcp->mc_has_assign) {
        cfg0 |= 1 << 7;
        pwm[pwm_count++] = anim_assign_brightness(msec, module_index);
    }
    
    packed_RGB rgb = anim_module_color(msec, module_index);
    uint8_t pix = mcp->mc_LED;
    pixels[pix][0] = rgb >> 16 & 0xFF;
    pixels[pix][1] = rgb >>  8 & 0xFF;
    pixels[pix][2] = rgb >>  0 & 0xFF;

    if (mcp->mc_flags & MCF_CTLRS)
        pixel_count = 2;
    else
        pixel_count = 1 + mcp->mc_knob_count;
    cfg1 = pixel_count | mcp->mc_knob_count << 3;
    
    for (size_t ki = 0; ki < mcp->mc_knob_count; ki++) {
        const knob_config *kcp = &mcp->mc_knobs[ki];
        if (kcp->kc_name) {     // if knob exists...
            packed_RGB rgb = anim_knob_color(msec, module_index, ki);
            uint8_t pix = kcp->kc_LED;
            pixels[pix][0] = rgb >> 16 & 0xFF;
            pixels[pix][1] = rgb >>  8 & 0xFF;
            pixels[pix][2] = rgb >>  0 & 0xFF;
        }
    }

    // Assemble the packet.
    uint8_t *p = packet;
    *p++ = STX;
    *p++ = cfg0;
    *p++ = cfg1;
    for (size_t i = 0; i < pwm_count; i++)
        *p++ = pwm[i];
    for (size_t i = 0; i < pixel_count; i++) {
        *p++ = pixels[i][0];
        *p++ = pixels[i][1];
        *p++ = pixels[i][2];
    }
    uint16_t cksum = fletcher16(packet + 1, p - packet - 1);
    *p++ = cksum >> 8;
    *p++ = cksum & 0xFF;
    *p++ = ETX;
    *p++ = SYN;
    assert(p - packet <= MAX_SPI_PACKET_SIZE);

    return p - packet;
}
