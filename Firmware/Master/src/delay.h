#ifndef DELAY_included
#define DELAY_included

#include <stdint.h>

extern void init_delay(uint32_t cpu_freq);

extern void delay_usec(uint32_t count);

#endif /* !DELAY_included */
