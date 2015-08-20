#ifndef SPI_PROTO_included
#define SPI_PROTO_included

#include <stdint.h>

#define MAX_SPI_PACKET_SIZE 25

const uint8_t STX = '\02';
const uint8_t ETX = '\03';
const uint8_t SYN = '\26';

typedef uint8_t spi_buf[MAX_SPI_PACKET_SIZE];

extern void assemble_outgoing_packet();

#endif /* !SPI_PROTO_included */
