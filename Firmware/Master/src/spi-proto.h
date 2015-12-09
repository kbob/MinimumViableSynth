#ifndef SPI_PROTO_included
#define SPI_PROTO_included

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

static const int     NO_GROUP = -1;
static const int     NO_BUS   = -1;

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
    uint8_t ss_buttons;         // ss_button_bit
    uint8_t ss_analog_mask;
    uint8_t ss_analog_values[MAX_KNOBS];
} slave_state;

typedef void SPI_slave_state_handler(size_t module_index,
                                     slave_state const *,
                                     void *user_data);

extern void SPI_proto_setup(void);
extern void SPI_proto_register_slave_state_handler(SPI_slave_state_handler *,
                                                   void *user_data);
extern void SPI_proto_report_and_clear_stats(void);

// Use these like this:
//
//   for (i = 0; (g = active_spi_groups(i)) != NO_GROUP; i++) {
//       for (j = 0; (b = active_spi_buses(g, j)) != NO_BUS; j++) {
//           size_t m = spi_to_module(g, b);
//           do something with module m, group g, bus b.
//       }
//   }

extern int active_spi_groups(uint8_t index);
extern int active_spi_buses(uint8_t group, uint8_t index);
extern size_t spi_to_module(int spi_group, int spi_bus);

#endif /* !SPI_PROTO_included */
