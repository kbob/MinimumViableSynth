#include "state.h"

#include <string.h>

synth_state ss;

static synth_state ss_factory_defaults = {
    .ss_preset_name = "Factory Defaults",
    .ss_preset_number = 0,
    .ss_MIDI_channel = 1,
    .ss_modules[M_LFO1] = {
        .ms_is_active = true,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 0,
        },
        .ms_knobs[K_LFO1_SPD] = {
            .ks_exported_value = 64, // XXX midi value for 3.0 Hz
            .ks_should_export = false,
        },
        .ms_knobs[K_LFO1_AMT] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_assign = {
            .as_index = DEST_LFO1_OSC1_PIT,
        },
    },
    .ss_modules[M_LFO2] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 1,
        },
        .ms_knobs[K_LFO2_SPD] = {
            .ks_exported_value = 64, // XXX midi value for 3.0 Hz
            .ks_should_export = false,
        },
        .ms_knobs[K_LFO2_AMT] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_assign = {
            .as_index = DEST_LFO2_OFF,
        },
    },
    .ss_modules[M_CTLRS] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 1,
        },
        .ms_knobs[K_LFO2_SPD] = {
            .ks_exported_value = 64, // XXX midi value for 3.0 Hz
            .ks_should_export = false,
        },
        .ms_knobs[K_LFO2_AMT] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_assign = {
            .as_index = DEST_LFO2_OFF,
        },
    },
    .ss_modules[M_OSC1] = {
    },
    .ss_modules[M_OSC2] = {
    },
    .ss_modules[M_NOIS] = {
    },
    .ss_modules[M_MIX] = {
    },
    .ss_modules[M_FILT] = {
    },
    .ss_modules[M_ENV1] = {
    },
    .ss_modules[M_ENV2] = {
    },
    .ss_modules[M_ENV3] = {
    },
};

void init_synth_state(void)
{
    memcpy(&ss, &ss_factory_defaults, sizeof ss);
}
