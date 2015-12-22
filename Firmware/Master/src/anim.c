#include "anim.h"

#include <assert.h>
#include <stdio.h>              // XXX

#include "config.h"
#include "spi-responder.h"

// We need phase/intensity for several things.
//
// Gentle throb for active module
//
// Fast throb for selected assignment src/dest
//  maybe 5x/second
//
// Slow throb for unselected assignment destinations
//  maybe every 3rd fast throb

#define GENTLE_PERIOD_MSEC 2000
#define GENTLE_MIN_LEVEL     20
#define GENTLE_MAX_LEVEL    255

#define DIM_LEVEL            40

#define AA_PERIOD_MSEC      200
#define AA_PERIOD_MSEC    200
#define AA_ON_MSEC        200

#define IA_PERIOD_MSEC     100
#define IA_ON_MSEC         90

static uint16_t gentle_mult;
static uint16_t active_assign_mult;
static uint16_t inactive_assign_mult;

static uint8_t biramp(uint32_t i, uint32_t n)
{
    if (i >= n/2)
        i = n - i - 1;
    if (i >= n/2)
        return 0;
    return i * 255 / (n/2 - 1);
}

void anim_update(uint32_t msec)
{
    // Calculate the multiplier for the gentle throb.

    static uint32_t gentle_period_start;
    uint32_t period = GENTLE_PERIOD_MSEC;
    uint32_t min = GENTLE_MIN_LEVEL;
    uint32_t max = GENTLE_MAX_LEVEL;
    uint32_t phase = msec - gentle_period_start;
    if (phase >= period) {
        phase -= period;
        gentle_period_start += period;
    }
    if (phase >= period / 2)
        phase = period - phase;
    uint32_t level = min + ((max - min) * 2 * phase) / period;
    gentle_mult = level;
    assert(gentle_mult >= GENTLE_MIN_LEVEL);
    assert(gentle_mult <= GENTLE_MAX_LEVEL);


    // Calculate the multiplier for the fast throb.

    static uint32_t aa_period_start;
    period = AA_PERIOD_MSEC;
    phase = msec - aa_period_start;
    if (phase > period) {
        phase -= period;
        aa_period_start += period;
    }
    if (phase < AA_ON_MSEC)
        active_assign_mult = 255;
    else
        active_assign_mult = 0;


    // Calculate the multiplier for the slow throb.

    static uint32_t ia_period_start;
    period = IA_PERIOD_MSEC;
    phase = msec - ia_period_start;
    if (phase > period) {
        phase -= period;
        ia_period_start += period;
    }
    if (phase < IA_ON_MSEC)
        inactive_assign_mult = biramp(phase, AA_ON_MSEC);
    else
        inactive_assign_mult = 0;
}

// The light for a module should be:
//
//   - module color with fast flashing if the module is the current
//     assign source.
//
//   - module color with throbbing if the module is active and current.
//
//   - module color, dimmed, if the module is active
//     but not current.
//
//   - module color, dimmed and throbbing, if the module is current
//     but not active.
//
//   - off, if the module is neither active nor current.

packed_RGB anim_module_color(uint32_t msec, size_t module_index)
{
    const uint8_t *p = sc.sc_modules[module_index].mc_color;
    uint32_t r = p[0];
    uint32_t g = p[1];
    uint32_t b = p[2];

    if (module_is_active_assign_source(module_index)) {
        r = (r * active_assign_mult) >> 8;
        g = (g * active_assign_mult) >> 8;
        b = (b * active_assign_mult) >> 8;
    } else {
        bool is_current = module_is_current(module_index);
        bool is_active = module_is_active(module_index);

        if (is_current && is_active) {
            r = (r * gentle_mult) >> 8;
            g = (g * gentle_mult) >> 8;
            b = (b * gentle_mult) >> 8;
        } else if (is_active) {
            r = (r * DIM_LEVEL) >> 8;
            g = (g * DIM_LEVEL) >> 8;
            b = (b * DIM_LEVEL) >> 8;
        } else if (is_current) {
            r = (r * gentle_mult * DIM_LEVEL) >> 16;
            g = (g * gentle_mult * DIM_LEVEL) >> 16;
            b = (b * gentle_mult * DIM_LEVEL) >> 16;
        } else {
            r = g = b = 0;
        }
    }
    return r << 16 | g << 8 | b << 0;
}

// The light for a knob should be:
//
// Assignment:
//
//   - solid red if a modifier assignment is active and the knob is the
//     modifier's current destination.
//
//   - blinking red if a modifier assignment is active and the knob
//     is a valid destination for the source.
//
//   - off if a modifier assignment is active and the knob is
//     not a valid destination.
//
//   - throbbing red if a modifier assignment is confirming and
//     the knob is the new destination.
//
// Normally:
//
//   - off if the module is currently inactive.
//
//   - off if the knob is idle.
//
//   - module color if the knob is working and current.
//
//   - dimmed module color if the knob is working and not current.

packed_RGB anim_knob_color(uint32_t msec,
                           size_t   module_index,
                           size_t   knob_index)
{
    const module_config *mc = &sc.sc_modules[module_index];
    const knob_config *kc = &mc->mc_knobs[knob_index];
    const uint8_t *p = kc->kc_color;
    uint32_t r = p[0];
    uint32_t g = p[1];
    uint32_t b = p[2];
    if (assignment_is_active()) {
        if (knob_is_active_assign_dest(module_index, knob_index)) {
            r = (0xFF * active_assign_mult) >> 8;
            g = 0;
            b = 0;
        } else if (knob_is_assign_dest(module_index, knob_index)) {
            r = (0xFF * inactive_assign_mult) >> 8;
            g = 0;
            b = 0;
        } else
            r = g = b = 0;
    } else if (assignment_is_confirmed() &&
               knob_is_active_assign_dest(module_index, knob_index)) {
        r = 0xFF;
        g = 0;
        b = 0;
    } else if (!module_is_active(module_index)) {
        r = g = b = 0;
    } else if (!knob_is_working(module_index, knob_index)) {
        r = g = b = 0;
    // } else if (!knob_is_current(module_index, knob_index)) {
    //     r = (r * DIM_LEVEL) >> 8;
    //     g = (g * DIM_LEVEL) >> 8;
    //     b = (b * DIM_LEVEL) >> 8;
    }

    return r << 16 | g << 8 | b << 0;
}

uint8_t anim_choice_brightness(uint32_t msec, size_t module_index)
{
    return 0x33;
}

uint8_t anim_assign_brightness(uint32_t msec, size_t module_index)
{
    if (assignment_is_active()) {
        if (module_is_active_assign_source(module_index))
            return (active_assign_mult * 0xFF) >> 8;
        else
            return 0;
    } else {
        if (source_is_assigned(module_index))
            return 0x0F;
        else
            return 0;
    }
}
