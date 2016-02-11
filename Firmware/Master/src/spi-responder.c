#include "spi-responder.h"

#include <assert.h>
#include <stdio.h>
#include <stddef.h>

#include "config.h"
#include "midi.h"
#include "modes.h"
#include "spi-proto.h"
#include "state.h"

static void handle_choice(size_t module_index)
{
    module_config const *mc = &sc.sc_modules[module_index];
    module_state        *ms = &ss.ss_modules[module_index];

    cancel_assignment();
    set_current_knob(module_index, K_NONE);
    uint8_t value = ms->ms_choice.cs_value + 1;
    if (value >= mc->mc_choice.cc_count)
        value = 0;
    ms->ms_choice.cs_value = value;
    MIDI_send_control_change(MIDI_default_channel, mc->mc_choice.cc_CC, value);
}

static void handle_assign(size_t module_index)
{
    if (assignment_is_active()) {
        if (module_index == active_source_mod_index()) {
            set_current_knob(M_NONE, K_NONE);
            cancel_assignment();
        } else {
            set_current_knob(module_index, K_NONE);
            begin_assignment(module_index);
        }
    } else {
        set_current_knob(module_index, K_NONE);
        begin_assignment(module_index);
    }
}

static void send_MIDI_assign(size_t src_mod_idx,
                             size_t dst_mod_idx,
                             size_t dst_knob_idx)
{
    module_config const *src_mc = &sc.sc_modules[src_mod_idx];
    assign_config const *ac     = &src_mc->mc_assign;
    size_t dest_idx = dest_index_by_knob(src_mod_idx,
                                         dst_mod_idx, dst_knob_idx);
    assert(dest_idx != D_NONE);
    assign_dest const *ad = &ac->ac_dests[dest_idx];
    ss.ss_modules[src_mod_idx].ms_assign.as_index = dest_idx;
    MIDI_send_control_change(MIDI_default_channel, ac->ac_CC, ad->ad_CC_val);
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
            size_t src_mod_index = active_source_mod_index();
            size_t dest_mod_index, dest_knob_index;
            if (knob_is_active_assign_dest(module_index, knob_index)) {
                dest_mod_index = M_NONE;
                dest_knob_index = K_NONE;
            } else {
                dest_mod_index = module_index;
                dest_knob_index = knob_index;
            }
            confirm_assignment(dest_mod_index, dest_knob_index);
            send_MIDI_assign(src_mod_index, dest_mod_index, dest_knob_index);
        } else
            cancel_assignment();
    } else {
        set_current_knob(module_index, knob_index);
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

    if (ks->ks_should_export)
        set_current_knob(module_index, knob_index);
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
