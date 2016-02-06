#include "spi-responder.h"

#include <assert.h>
#include <stdio.h>              // XXX

#include "config.h"
#include "midi.h"
#include "spi-proto.h"
#include "state.h"
#include "systick.h"

#define ASSIGN_TIMEOUT_MSEC  2000
#define CONFIRM_TIMEOUT_MSEC  500

typedef enum AssignmentState {
    AS_INACTIVE,
    AS_ACTIVE,
    AS_CONFIRMING,          // when the confirming animation runs.
} AssignmentState;

static AssignmentState a_state;
static uint32_t a_start_time;
static uint32_t a_confirm_time;
static size_t   a_src_mod_index      = M_NONE;
static size_t   a_dest_mod_index     = M_NONE;
static size_t   a_dest_knob_index    = K_NONE;
static size_t   current_module_index = M_NONE;
static size_t   current_knob_index   = K_NONE;

bool module_is_current(size_t module_index)
{
    return module_index == current_module_index;
}

bool module_is_active(size_t module_index)
{
    return true;                // XXX do this right.
}

bool knob_is_current(size_t module_index, size_t knob_index)
{
    return (module_index == current_module_index &&
            knob_index == current_knob_index);
}

bool knob_is_working(size_t module_index, size_t knob_index)
{
    const knob_state *ks = &ss.ss_modules[module_index].ms_knobs[knob_index];
    return ks->ks_should_export;
}

bool module_is_active_assign_source(size_t module_index)
{
    return module_index == a_src_mod_index;
}

bool knob_is_active_assign_dest(size_t module_index, size_t knob_index)
{
    return module_index == a_dest_mod_index && knob_index == a_dest_knob_index;
}

bool knob_is_assign_dest(size_t module_index, size_t knob_index)
{
    if (a_src_mod_index == M_NONE)
        return false;
    size_t dest_idx = dest_index_by_knob(a_src_mod_index,
                                         module_index, knob_index);
    return dest_idx != D_NONE;
}

static void begin_assignment(size_t src_mod_index)
{
    module_config const *src_mc     = &sc.sc_modules[src_mod_index];
    module_state  const *src_ms     = &ss.ss_modules[src_mod_index];
    assign_config const *ac         = &src_mc->mc_assign;
    assign_dest   const *dest       = &ac->ac_dests[src_ms->ms_assign.as_index];
    assert(src_mc->mc_has_assign);
    a_state = AS_ACTIVE;
    a_start_time = system_millis;
    a_confirm_time = 0;
    a_src_mod_index = src_mod_index;
    a_dest_mod_index = dest->ad_module;
    a_dest_knob_index = dest->ad_control;
}

static void confirm_assignment(size_t dest_mod_index, size_t dest_knob_index)
{
    a_state = AS_CONFIRMING;
    a_start_time = 0;
    a_confirm_time = system_millis;
    a_dest_mod_index = dest_mod_index;
    a_dest_knob_index = dest_knob_index;

    // printf("Confirm %d.%d\n", dest_mod_index, dest_knob_index);
    module_config const *src_mc = &sc.sc_modules[a_src_mod_index];
    assign_config const *ac     = &src_mc->mc_assign;
    size_t dest_idx = dest_index_by_knob(a_src_mod_index,
                                         dest_mod_index, dest_knob_index);
    assert(dest_idx != D_NONE);
    assign_dest const *ad = &ac->ac_dests[dest_idx];
    ss.ss_modules[a_src_mod_index].ms_assign.as_index = dest_idx;
    MIDI_send_control_change(MIDI_default_channel, ac->ac_CC, ad->ad_CC_val);
}

static void cancel_assignment()
{
    a_state = AS_INACTIVE;
    a_start_time = 0;
    a_confirm_time = 0;
    a_src_mod_index = M_NONE;
    a_dest_mod_index = M_NONE;
    a_dest_knob_index = K_NONE;
}

bool source_is_assigned(size_t module_index)
{
    module_config const *mc = &sc.sc_modules[module_index];
    module_state  const *ms = &ss.ss_modules[module_index];
    assign_config const *ac = &mc->mc_assign;
    assign_state  const *as = &ms->ms_assign;
    assert(mc->mc_has_assign);
    assert(as->as_index < ac->ac_dest_count);
    return ac->ac_dests[as->as_index].ad_module != M_NONE;
}

bool assignment_is_active(void)
{
    if (a_state == AS_ACTIVE &&
        system_millis - a_start_time > ASSIGN_TIMEOUT_MSEC)
    {
        cancel_assignment();
    }
    return a_state == AS_ACTIVE;
}

bool assignment_is_confirmed(void)
{
    if (a_state == AS_CONFIRMING &&
        system_millis - a_confirm_time > CONFIRM_TIMEOUT_MSEC)
    {
        cancel_assignment();
    }
    return a_state == AS_CONFIRMING;
}

static void handle_choice(size_t module_index)
{
    module_config const *mc = &sc.sc_modules[module_index];
    module_state        *ms = &ss.ss_modules[module_index];

    cancel_assignment();
    current_module_index = module_index;
    current_knob_index = K_NONE;
    uint8_t value = ms->ms_choice.cs_value + 1;
    if (value >= mc->mc_choice.cc_count)
        value = 0;
    ms->ms_choice.cs_value = value;
    MIDI_send_control_change(MIDI_default_channel, mc->mc_choice.cc_CC, value);
}

static void handle_assign(size_t module_index)
{
    switch (a_state) {

    case AS_INACTIVE:
    case AS_CONFIRMING:
        current_module_index = module_index;
        current_knob_index = K_NONE;
        begin_assignment(module_index);
        break;

    case AS_ACTIVE:
        if (module_index == a_src_mod_index) {
            current_module_index = M_NONE;
            current_knob_index = K_NONE;
            cancel_assignment();
        } else {
            current_module_index = module_index;
            current_knob_index = K_NONE;
            begin_assignment(module_index);
        }
        break;
    }
}

static void handle_knob_button(size_t module_index,
                               size_t knob_index)
{
    const knob_config *kc = &sc.sc_modules[module_index].mc_knobs[knob_index];
    printf("Button %s.%s pressed\n",
           sc.sc_modules[module_index].mc_name,
           kc->kc_name);

    if (assignment_is_active()) {
        if (knob_is_assign_dest(module_index, knob_index)) {
            if (knob_is_active_assign_dest(module_index, knob_index)) {
                confirm_assignment(M_NONE, K_NONE);
            } else {
                confirm_assignment(module_index, knob_index);
            }
        } else {
            cancel_assignment();
        }
    } else {
        current_module_index = module_index;
        current_knob_index = knob_index;
        if (kc->kc_flags & KCF_PITCH) {
            // XXX do something here
        }
    }
}

static void handle_knob(size_t  module_index,
                        size_t  knob_index,
                        uint8_t new_value)
{
    assert(module_index < MODULE_COUNT);
    module_config const *mc = &sc.sc_modules[module_index];
    module_state        *ms = &ss.ss_modules[module_index];
    assert(knob_index < mc->mc_knob_count);
    knob_config   const *kc = &mc->mc_knobs[knob_index];
    knob_state          *ks = &ms->ms_knobs[knob_index];

    // cancel_assignment();
    current_module_index = module_index;
    current_knob_index = knob_index;

    uint8_t old_value = ks->ks_actual_value;
    // printf("Knob %s.%s changed %u -> %u\n",
    //        mc->mc_name, kc->kc_name, old_value, new_value);
    if (!ks->ks_should_export) {
        if (old_value < ks->ks_exported_value) {
            if (new_value >= ks->ks_exported_value)
                ks->ks_should_export = true;
        } else {
            if (new_value <= ks->ks_exported_value)
                ks->ks_should_export = true;
        }
    }
    ks->ks_actual_value = new_value;

    uint8_t CC_value = new_value / 2;
    if (ks->ks_should_export && CC_value != ks->ks_exported_value) {
        MIDI_send_control_change(MIDI_default_channel, kc->kc_CC_msb, CC_value);
        ks->ks_exported_value = CC_value;
    }
}

static void handle_slave_state(size_t             module_index,
                               slave_state const *sls,
                               void              *user_data)
{
    const module_config *mc = &sc.sc_modules[module_index];

    if (sls->ss_buttons & SBB_CHOICE)
        handle_choice(module_index);

    if (sls->ss_buttons & SBB_ASSIGN)
        handle_assign(module_index);

    for (size_t i = 0; i < mc->mc_knob_count; i++) {
        const knob_config *kc = &mc->mc_knobs[i];
        if (!kc->kc_name)       // knob does not exist; skip it.
            continue;
        if (kc->kc_has_button && (sls->ss_buttons & ((1 << 2) << i)))
            handle_knob_button(module_index, i);
        if (sls->ss_analog_mask & (1 << i))
            handle_knob(module_index, i, sls->ss_analog_values[i]);
    }
}

void SPI_responder_setup(void)
{
    cancel_assignment();
    SPI_proto_register_slave_state_handler(handle_slave_state, NULL);
}
