#include "anim.h"

#include "config.h"

packed_RGB anim_module_color(uint32_t msec, size_t module_index)
{
    // XXX minimal version
    const uint8_t *p = sc.sc_modules[module_index].mc_color;
    uint32_t r = p[0];
    uint32_t g = p[1];
    uint32_t b = p[2];
    uint32_t m = msec % 2000;
    if (m > 1000)
        m = 2000 - m;
    r *= m; r /= 2000;
    g *= m; g /= 2000;
    b *= m; b /= 2000;
    return r << 16 | g << 8 | b << 0;
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
    uint32_t m = msec % 2000;
    if (m > 1000)
        m = 2000 - m;
    return m * 0xFF / 1000;
    // return 0xFF;
}

uint8_t anim_assign_brightness(uint32_t msec, size_t module_index)
{
    return msec % 2000 < 200 ? 0xFF : 0x00;
}
