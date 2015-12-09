#ifndef SPI_included
#define SPI_included

#include <stdint.h>
#include <stddef.h>

#define SPI_GROUP_COUNT 3
#define SPI_BUS_RANGE   7       // bus numbers are [1 .. RANGE)
#define SPI_BUS_COUNT   4       // four buses actually in use

typedef void spi_completion_handler(void);

extern void spi_setup(void);
extern void spi_register_completion_handler(spi_completion_handler *);

extern void spi_select_group(int group);
extern void spi_deselect_group(int group);

extern void spi_start_transfer(int            spi,
                               uint8_t const *tx_buf,
                               uint8_t       *rx_buf,
                               size_t         count);

extern void spi_finish_transfer(int spi);

#endif /* !SPI_included */
