#include "delay.h"

#include "systick.h"

// If compiled with no optimization (-O0), delay_loop uses 19 cycles
// per iteration.  With optimization (-O3), it uses 4 cycles.

#define FAST_CYCLES        4
#define SLOW_CYCLES       19
#define uSEC_per_SEC 1000000

static uint32_t delay_scale;

static inline void delay_loop(uint32_t count)
{
    while (count--)
        __asm__ volatile ( "nop" );
}

void init_delay(uint32_t cpu_freq)
{
    // Assume fast.
    delay_scale = cpu_freq / FAST_CYCLES / uSEC_per_SEC;

    // Test.
    uint32_t b = system_millis;
    delay_loop(1000 * delay_scale);
    uint32_t a = system_millis;

    if (a - b >= 2) {
        // Slow.
        delay_scale = cpu_freq / SLOW_CYCLES / uSEC_per_SEC;
    }
}

void delay_usec(uint32_t count)
{
    delay_loop(count * delay_scale);
}
