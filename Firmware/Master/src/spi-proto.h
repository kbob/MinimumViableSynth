#ifndef SPI_PROTO_included
#define SPI_PROTO_included

// XXX This file needs to be organized.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

// #define MAX_SPI_PACKET_SIZE 32

// static const uint8_t STX      = '\02';
// static const uint8_t ETX      = '\03';
// static const uint8_t SYN      = '\26';

static const int     NO_GROUP = -1;
static const int     NO_BUS   = -1;

// typedef uint8_t spi_buf[MAX_SPI_PACKET_SIZE];

typedef enum ss_button_bit {
    SBB_CHOICE = 1 << 0,
    SBB_ASSIGN = 1 << 1,
    SBB_DEST_1 = 1 << 2,
    SBB_DEST_2 = 1 << 3,
    SBB_DEST_3 = 1 << 4,
    SBB_DEST_4 = 1 << 5,
} ss_button_bit;

typedef struct slave_state {
    bool    ss_is_valid;
    uint8_t ss_buttons;
    uint8_t ss_analog_mask;
    uint8_t ss_analog_values[MAX_KNOBS];
} slave_state;

typedef void SPI_slave_state_handler(size_t module_index,
                                     slave_state const *,
                                     void *user_data);

extern void SPI_proto_setup(void);
extern void SPI_register_slave_state_handler(SPI_slave_state_handler);
extern void SPI_report_and_clear_stats(void);

// extern size_t assemble_outgoing_packet(spi_buf  packet,
//                                        uint32_t msec,
//                                        size_t   module_index);

// extern bool parse_incoming_packet(spi_buf const packet,
//                                   size_t        count,
//                                   size_t        module_index,
//                                   slave_state  *state_out);

extern size_t spi_to_module(uint8_t spi_group, uint8_t spi_bus);

// Use these like this:
//
//   for (i = 0; (g = active_spi_groups(i)) != NO_GROUP; i++)
//       for (j = 0; (b = active_spi_buses(g, j)) != NO_BUS; j++)
//           do something with group g, bus b.

extern int active_spi_groups(uint8_t index);
extern int active_spi_buses(uint8_t group, uint8_t index);

#endif /* !SPI_PROTO_included */
