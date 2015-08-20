#include "spi-proto.h"

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

#define SPI_GROUP_COUNT 3
#define SPI_BUS_COUNT 7

static void module_to_spi(size_t   module_index,
                          uint8_t *spi_group_out,
                          uint8_t *spi_bus_out)
{
    assert(module_index < MODULE_COUNT);
    module_config *mcp = &sc.sc_modules[module_index];
    if (spi_group_out)
        *spi_group_out = mcp->mc_spi_group;
    if (spi_bus_out)
        *spi_bus_out = mcp->mc_spi_bus;
}

static size_t spi_to_module(uint8_t spi_group, uint8_t spi_bus)
{
    static size_t spi2mod[SPI_GROUP_COUNT][SPI_BUS_COUNT];
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

void assemble_outgoing_packet(spi_buf *packet, size_t module_index)
{
    uint8_t *p = packet;
    const module_config *mcp = &sc.sc_modules[module_index];
    const module_state *msp = &ss.ss_modules[module_index];

    *p++ = STX;
    *p++ = 
}
