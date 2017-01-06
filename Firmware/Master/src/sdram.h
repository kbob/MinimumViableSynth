#ifndef SDRAM_included
#define SDRAM_included

#define SDRAM_BASE_ADDRESS 0xd0000000

void sdram_setup(void);

// variable attribute to put global/static variables into SDRAM.
// SDRAM is initialized to zero, so don't use with initialized variables.
#define SDRAM_SECTION __attribute((section(".sdram")))

#endif /* !SDRAM_included */
