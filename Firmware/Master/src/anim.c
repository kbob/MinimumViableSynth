#include "anim.h"

#include <assert.h>

#include "config.h"
#include "modes.h"

#define MOD_DIM_LEVEL            40
#define KNOB_DIM_LEVEL           90
#define CHOICE_LEVEL             50
#define ASS_BUTTON_DIM_LEVEL     30
#define ASS_BUTTON_BRIGHT_LEVEL 255

#define ASSIGN_THROB_PERIOD     200
#define CUR_MOD_THROB_PERIOD   2000
#define ASSIGN_FADE_TIME        500

typedef struct anim_effect {
    uint8_t ae_m[3];
    uint8_t ae_b[3];
} anim_effect;

typedef struct throbber {
    uint32_t const t_period;
    uint8_t  const t_min;
    uint8_t  const t_max;
    uint32_t       t_origin;
} throbber;

typedef struct fader {
    uint32_t const f_duration;
    uint32_t const f_delay;
} fader;

static const anim_effect null_effect = {
    .ae_m = { 255, 255, 255 },
    .ae_b = {   0,   0,   0 },
};

static const anim_effect dark_effect = {
    .ae_m = {   0,   0,   0 },
    .ae_b = {   0,   0,   0 },
};

static throbber assign_throbber = {
    .t_period   = ASSIGN_THROB_PERIOD,
    .t_min      = 20,
    .t_max      = 255,
};

static throbber cur_mod_throbber = {
    .t_period   = CUR_MOD_THROB_PERIOD,
    .t_min      = MOD_DIM_LEVEL,
    .t_max      = 255,
};

static fader assign_fader = {
    .f_duration = ASSIGN_FADE_TIME,
    .f_delay    = ASSIGN_TIMEOUT_MSEC - ASSIGN_FADE_TIME
};

static anim_effect       active_source_mod_effect;
static anim_effect         active_dest_mod_effect;
static anim_effect     inactive_assign_mod_effect;
static anim_effect      current_active_mod_effect;
static anim_effect              active_mod_effect;
static anim_effect    current_inactive_mod_effect;
static anim_effect            inactive_mod_effect;

static anim_effect        active_dest_knob_effect;
static anim_effect         valid_dest_knob_effect;
static anim_effect           non_dest_knob_effect;
static anim_effect     confirmed_dest_knob_effect;
static anim_effect confirmed_non_dest_knob_effect;
static anim_effect    current_working_knob_effect;
static anim_effect            working_knob_effect;
static anim_effect               idle_knob_effect;

static uint8_t         active_assign_button_level;
static uint8_t       inactive_assign_button_level;
static uint8_t      confirmed_assign_button_level;
static uint8_t       assigned_assign_button_level;
static uint8_t     unassigned_assign_button_level;

static packed_RGB apply_effect(const anim_effect *ae,
                               const uint8_t mod_color[3])
{
    uint32_t r = (uint32_t)ae->ae_m[0] * mod_color[0] / 255 + ae->ae_b[0];
    uint32_t g = (uint32_t)ae->ae_m[1] * mod_color[1] / 255 + ae->ae_b[1];
    uint32_t b = (uint32_t)ae->ae_m[2] * mod_color[2] / 255 + ae->ae_b[2];
    return r << 16 | g << 8 | b << 0;
}

static uint8_t throb(uint32_t msec, throbber *thr)
{
    uint32_t phase = msec - thr->t_origin;
    uint32_t period = thr->t_period;
    if (phase >= period) {
        uint32_t adjust = phase - phase % period;
        thr->t_origin += adjust;
        phase -= adjust;
    }
    assert(0 <= phase && phase < thr->t_period);
    if (phase >= period / 2)
        phase = period - phase;
    uint8_t min = thr->t_min;
    uint8_t mag = thr->t_max - min;
    return min + phase * mag * 2 / period;
}

static uint8_t fade(uint32_t msec, fader *fad)
{
    if (msec < fad->f_delay)
        return 255;
    if (msec > fad->f_delay + fad->f_duration)
        return 0;
    return 255 - (msec - fad->f_delay) * 255 / fad->f_duration;
}

void anim_update(uint32_t msec)
{
    if (assignment_is_active()) {

        uint8_t a_throb = throb(msec, &assign_throbber);
        uint8_t a_fade = fade(msec - assignment_start_time(), & assign_fader);
        uint8_t a_mult = a_throb * a_fade / 255;

        size_t src_mod_idx = active_source_mod_index();
        assert(src_mod_idx != M_NONE);
        const uint8_t *src_color = sc.sc_modules[src_mod_idx].mc_color;

        // source module's solid color
        active_source_mod_effect.ae_m[0] = 0;
        active_source_mod_effect.ae_m[1] = 0;
        active_source_mod_effect.ae_m[2] = 0;
        active_source_mod_effect.ae_b[0] = src_color[0];
        active_source_mod_effect.ae_b[1] = src_color[1];
        active_source_mod_effect.ae_b[2] = src_color[2];

        // throb red
        active_assign_button_level = a_mult;

        // throb source module's color
        active_dest_mod_effect.ae_m[0] = 0;
        active_dest_mod_effect.ae_m[1] = 0;
        active_dest_mod_effect.ae_m[2] = 0;
        active_dest_mod_effect.ae_b[0] = src_color[0] * a_mult / 255;
        active_dest_mod_effect.ae_b[1] = src_color[1] * a_mult / 255;
        active_dest_mod_effect.ae_b[2] = src_color[2] * a_mult / 255;

        active_dest_knob_effect.ae_m[0] = 0;
        active_dest_knob_effect.ae_m[1] = 0;
        active_dest_knob_effect.ae_m[2] = 0;
        active_dest_knob_effect.ae_b[0] = src_color[0] * a_mult / 255;
        active_dest_knob_effect.ae_b[1] = src_color[1] * a_mult / 255;
        active_dest_knob_effect.ae_b[2] = src_color[2] * a_mult / 255;

        // solid red
        valid_dest_knob_effect.ae_m[0] = 0;
        valid_dest_knob_effect.ae_m[1] = 0;
        valid_dest_knob_effect.ae_m[2] = 0;
        valid_dest_knob_effect.ae_b[0] = a_fade;
        valid_dest_knob_effect.ae_b[1] = 0;
        valid_dest_knob_effect.ae_b[2] = 0;

        // dark
        inactive_assign_mod_effect = dark_effect;
        non_dest_knob_effect = dark_effect;
        inactive_assign_button_level = 0;

    } else {

        if (assignment_is_confirmed()) {

            size_t src_mod_idx = active_source_mod_index();
            assert(src_mod_idx != M_NONE);
            const uint8_t *src_color = sc.sc_modules[src_mod_idx].mc_color;

            confirmed_dest_knob_effect.ae_m[0] = 0;
            confirmed_dest_knob_effect.ae_m[1] = 0;
            confirmed_dest_knob_effect.ae_m[2] = 0;
            confirmed_dest_knob_effect.ae_b[0] = src_color[0];
            confirmed_dest_knob_effect.ae_b[1] = src_color[1];
            confirmed_dest_knob_effect.ae_b[2] = src_color[2];

            confirmed_non_dest_knob_effect = dark_effect;

            confirmed_assign_button_level = ASS_BUTTON_BRIGHT_LEVEL;
        }

        uint8_t cm_throb = throb(msec, &cur_mod_throbber);

        // throb bright module color
        current_active_mod_effect.ae_m[0] = cm_throb;
        current_active_mod_effect.ae_m[1] = cm_throb;
        current_active_mod_effect.ae_m[2] = cm_throb;
        current_active_mod_effect.ae_b[0] = 0;
        current_active_mod_effect.ae_b[1] = 0;
        current_active_mod_effect.ae_b[2] = 0;
        current_working_knob_effect.ae_m[0] = cm_throb;
        current_working_knob_effect.ae_m[1] = cm_throb;
        current_working_knob_effect.ae_m[2] = cm_throb;
        current_working_knob_effect.ae_b[0] = 0;
        current_working_knob_effect.ae_b[1] = 0;
        current_working_knob_effect.ae_b[2] = 0;

        // dim module color
        active_mod_effect.ae_m[0] = MOD_DIM_LEVEL;
        active_mod_effect.ae_m[1] = MOD_DIM_LEVEL;
        active_mod_effect.ae_m[2] = MOD_DIM_LEVEL;
        active_mod_effect.ae_b[0] = 0;
        active_mod_effect.ae_b[1] = 0;
        active_mod_effect.ae_b[2] = 0;
        working_knob_effect.ae_m[0] = KNOB_DIM_LEVEL;
        working_knob_effect.ae_m[1] = KNOB_DIM_LEVEL;
        working_knob_effect.ae_m[2] = KNOB_DIM_LEVEL;
        working_knob_effect.ae_b[0] = 0;
        working_knob_effect.ae_b[1] = 0;
        working_knob_effect.ae_b[2] = 0;
        assigned_assign_button_level = ASS_BUTTON_DIM_LEVEL;

        // throb dim module color
        current_inactive_mod_effect.ae_m[0] = MOD_DIM_LEVEL * cm_throb / 255;
        current_inactive_mod_effect.ae_m[1] = MOD_DIM_LEVEL * cm_throb / 255;
        current_inactive_mod_effect.ae_m[2] = MOD_DIM_LEVEL * cm_throb / 255;
        current_inactive_mod_effect.ae_b[0] = 0;
        current_inactive_mod_effect.ae_b[1] = 0;
        current_inactive_mod_effect.ae_b[2] = 0;

        // dark
        inactive_mod_effect = dark_effect;
        idle_knob_effect = dark_effect;
        unassigned_assign_button_level = 0;
    }
}

// The light for a module should be:
//
//   * if assignment is active,
//       - if module is active source, blink red.
//       - if module contains active destination, blink the source's color.
//       - otherwise dark.
//
//   * if module is active and current,
//      - throbbing module color.
//
//   * if module is active,
//      - module color, dimmed.
//
//   * otherwise,
//      - dark.

packed_RGB anim_module_color(size_t module_index)
{
    const anim_effect *ae = &null_effect;
    if (assignment_is_active()) {
        if (module_is_active_assign_source(module_index))
            ae = &active_source_mod_effect;
        else if (module_index == active_dest_mod_index())
            ae = &active_dest_mod_effect;
        else
            ae = &inactive_assign_mod_effect;
    } else {
        bool is_current = module_is_current(module_index);
        bool is_active = module_is_active(module_index);

        if (is_current && is_active)
            ae = &current_active_mod_effect;
        else if (is_current)
            ae = &current_inactive_mod_effect;
        else if (is_active)
            ae = &active_mod_effect;
        else
            ae = &inactive_mod_effect;
    }
    const uint8_t *mod_color = sc.sc_modules[module_index].mc_color;
    return apply_effect(ae, mod_color);
}

// The light for a knob should be:
//
//   * if assignment is active,
//      - if knob is active destination, blink the source's color.
//      - if knob is valid destination, solid red.
//      - otherswise dark.
//
//   * if assignment is confirmed,
//      - if knob is active destination, solid source color.
//      - otherwise dark.
//
//   * if knob is working,
//      - if knob is current, throbbing module color.
//      - otherwise dim module color.
//
//   * otherwise,
//      - dark.

packed_RGB anim_knob_color(size_t module_index, size_t knob_index)
{
    const anim_effect *ae = &null_effect;
    if (assignment_is_active()) {
        if (knob_is_active_assign_dest(module_index, knob_index))
            ae = &active_dest_knob_effect;
        else if (knob_is_assign_dest(module_index, knob_index))
            ae = &valid_dest_knob_effect;
        else
            ae = &non_dest_knob_effect;
    } else if (assignment_is_confirmed()) {
        if (knob_is_active_assign_dest(module_index, knob_index))
            ae = &confirmed_dest_knob_effect;
        else
            ae = &confirmed_non_dest_knob_effect;
    } else if (knob_is_working(module_index, knob_index)) {
        if (knob_is_current(module_index, knob_index))
            ae = &current_working_knob_effect;
        else
            ae = &working_knob_effect;
    } else
        ae = &idle_knob_effect;
    module_config const *mc = &sc.sc_modules[module_index];
    knob_config   const *kc = &mc->mc_knobs[knob_index];
    uint8_t       const *knob_color = kc->kc_color;
    return apply_effect(ae, knob_color);
}

uint8_t anim_choice_brightness(size_t module_index)
{
    return CHOICE_LEVEL;
}

uint8_t anim_assign_brightness(size_t module_index)
{
    if (assignment_is_active()) {
        if (module_is_active_assign_source(module_index))
            return active_assign_button_level;
        else
            return inactive_assign_button_level;
    } else if (assignment_is_confirmed()) {
        if (module_is_active_assign_source(module_index))
            return confirmed_assign_button_level;
        else
            return inactive_assign_button_level;
    } else {
        if (source_is_assigned(module_index))
            return assigned_assign_button_level;
        else
            return unassigned_assign_button_level;
    }
}
