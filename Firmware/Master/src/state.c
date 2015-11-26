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
        .ms_is_active = true,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 0,
        },
        .ms_knobs[K_OSC1_WID] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_OSC1_AMT] = {
            .ks_exported_value = 127,
            .ks_should_export = false,
        },
    },
    .ss_modules[M_OSC2] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 1,
        },
        .ms_knobs[K_OSC2_WID] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_OSC2_AMT] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
    },
    .ss_modules[M_NOIS] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 0,
        },
        .ms_knobs[K_NOIS_AMT] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
    },
    .ss_modules[M_MIX] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 0,
        },
    },
    .ss_modules[M_FILT] = {
        .ms_is_active = true,
        .ms_LED_pattern = 0,    // XXX
        .ms_choice = {
            .cs_value = 0,
        },
        .ms_knobs[K_FILT_FRQ] = {
            .ks_exported_value = 127,
            .ks_should_export = false,
        },
        .ms_knobs[K_FILT_RES] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_FILT_DRV] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_FILT_KTK] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
    },
    .ss_modules[M_ENV1] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_knobs[K_ENV1_ATK] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_DCY] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_SUS] = {
            .ks_exported_value = 127,
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_RLS] = {
            .ks_exported_value = 0,
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_AMT] = {
            .ks_exported_value = 64, // zero
            .ks_should_export = false,
        },
        .ms_assign = {
            .as_index = DEST_ENV1_OFF,
        },
    },
    .ss_modules[M_ENV2] = {
        .ms_is_active = false,
        .ms_LED_pattern = 0,    // XXX
        .ms_knobs[K_ENV1_ATK] = {
            .ks_exported_value = 64, // XXX 0.2 sec
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_DCY] = {
            .ks_exported_value = 31, // XXX 0.1 sec
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_SUS] = {
            .ks_exported_value = 10,
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_RLS] = {
            .ks_exported_value = 31, // XXX 0.1 sec
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_AMT] = {
            .ks_exported_value = 64, // zero
            .ks_should_export = false,
        },
    },
    .ss_modules[M_ENV3] = {
        .ms_is_active = true,
        .ms_LED_pattern = 0,    // XXX
        .ms_knobs[K_ENV1_ATK] = {
            .ks_exported_value = 7, // XXX 0.001 sec
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_DCY] = {
            .ks_exported_value = 15, // XXX 0.1 sec
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_SUS] = {
            .ks_exported_value = 127,
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_RLS] = {
            .ks_exported_value = 31, // XXX 0.5 sec
            .ks_should_export = false,
        },
        .ms_knobs[K_ENV1_AMT] = {
            .ks_exported_value = 127,
            .ks_should_export = false,
        },
    },
};

void init_synth_state(void)
{
    memcpy(&ss, &ss_factory_defaults, sizeof ss);
}
