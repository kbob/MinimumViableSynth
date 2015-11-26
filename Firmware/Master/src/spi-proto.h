#ifndef SPI_PROTO_included
#define SPI_PROTO_included

#include <stddef.h>
#include <stdint.h>

#define MAX_SPI_PACKET_SIZE 32

static const uint8_t STX      = '\02';
static const uint8_t ETX      = '\03';
static const uint8_t SYN      = '\26';

static const int     NO_GROUP = -1;
static const int     NO_BUS   = -1;

typedef uint8_t spi_buf[MAX_SPI_PACKET_SIZE];

extern size_t assemble_outgoing_packet(spi_buf  packet,
                                       uint32_t msec,
                                       size_t   module_index);

extern size_t spi_to_module(uint8_t spi_group, uint8_t spi_bus);

// Use these like this:
//
//   for (i = 0; (g = active_spi_groups(i)) != NO_GROUP; i++)
//       for (j = 0; (b = active_spi_buses(g, j)) != NO_BUS; j++)
//           do something with group g, bus b.

extern int active_spi_groups(uint8_t index);
extern int active_spi_buses(uint8_t group, uint8_t index);

#endif /* !SPI_PROTO_included */
