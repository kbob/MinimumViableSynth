#ifndef SPI_included
#define SPI_included

#include <stdint.h>
#include <stdlib.h>

extern void spi_setup();

extern void spi_select_group(int group);
extern void spi_deselect_group(int group);

extern void spi_start_transfer(int            spi,
                               uint8_t const *tx_buf,
                               uint8_t       *rx_buf,
                               size_t         count);

extern void spi_finish_transfer(int spi);

#endif /* !SPI_included */
