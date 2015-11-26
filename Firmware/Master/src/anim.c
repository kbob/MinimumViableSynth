#include "anim.h"

#include <inttypes.h>
#include <stdio.h>              // XXX

#include "config.h"

packed_RGB anim_module_color(uint32_t msec, size_t module_index)
{
    // XXX minimal version
    const uint8_t *p = sc.sc_modules[module_index].mc_color;
    // printf("%s(%" PRIu32 ", %u): returning %#x\n",
    //        __func__, msec, module_index,
    //        p[0] << 16 | p[1] << 8 | p[2] << 0);
    return p[0] << 16 | p[1] << 8 | p[2] << 0;
}

packed_RGB anim_knob_color(uint32_t msec,
                           size_t   module_index,
                           size_t   knob_index)
{
    // XXX minimal version
    const uint8_t *p = sc.sc_modules[module_index].mc_color;
    return p[0] << 16 | p[1] << 8 | p[2] << 0;
}

uint8_t anim_choice_brightness(uint32_t msec, size_t module_index)
{
    // I think it will be distracting to have the choice LEDs animate.
    // But if I want to animate them later, I'll do it here.
    return 0xFF;
}

uint8_t anim_assign_brightness(uint32_t msec, size_t module_index)
{
    return msec % 1000 < 100 ? 0xFF : 0x00;
}
