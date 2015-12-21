#ifndef STATE_included
#define STATE_included

#include "config.h"

typedef enum LED_pattern {
    LP_OFF,
    LP_ACTIVE,
    LP_CURRENT_DEST,
    LP_AVAIL_DEST,
} LED_pattern;

typedef struct choice_state {
    // const choice_config *cs_config;
    uint8_t      cs_value;
} choice_state;

typedef struct knob_state {
    uint16_t     ks_exported_value;
    uint16_t     ks_actual_value;
    bool         ks_should_export;
} knob_state;

typedef struct assign_state {
    uint8_t      as_index;
} assign_state;

typedef struct module_state {
    bool         ms_is_active;
    LED_pattern  ms_LED_pattern;
    choice_state ms_choice;
    knob_state   ms_knobs[MAX_KNOBS];
    assign_state ms_assign;
} module_state;

typedef struct synth_state {
    char        *ss_preset_name;
    int          ss_preset_number;
    uint8_t      ss_MIDI_channel;
    module_state ss_modules[MODULE_COUNT];
} synth_state;

extern synth_state ss;

extern void init_synth_state(void);

#endif /* !STATE_included */
