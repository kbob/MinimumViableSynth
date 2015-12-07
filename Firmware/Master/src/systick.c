#include "systick.h"

#include <assert.h>

#include <libopencm3/cm3/systick.h>

volatile uint32_t system_millis;
static systick_handler *current_handler;

// This is the systick_isr.
void sys_tick_handler(void)
{
    system_millis++;
    if (current_handler)
        (*current_handler)(system_millis);
}

void systick_setup(uint32_t cpu_freq)
{
    // set tick rate to 1 KHz.
    systick_set_reload(cpu_freq / 1000);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

extern void register_systick_handler(systick_handler *handler)
{
    assert(!current_handler);
    current_handler = handler;
}

