#ifndef CONFIG_included
#define CONFIG_included

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_KNOBS         6
#define MODULE_COUNT     11

// Module and Knob Indices
#define M_LFO1       0
#define K_LFO1_SPD   0
#define K_LFO1_AMT   1

#define M_LFO2       1
#define K_LFO2_SPD   0
#define K_LFO2_AMT   1

#define M_CTLRS      2
#define K_CTLRS_AMT  1

#define M_OSC1       3
#define K_OSC1_WID   0
#define K_OSC1_PIT   1
#define K_OSC1_AMT   2

#define M_OSC2       4
#define K_OSC2_WID   0
#define K_OSC2_PIT   1
#define K_OSC2_AMT   2

#define M_NOIS       5
#define K_NOIS_AMT   0

#define M_MIX        6

#define M_FILT       7
#define K_FILT_FRQ   0
#define K_FILT_RES   1
#define K_FILT_DRV   2
#define K_FILT_KTK   3

#define M_ENV1       8
#define K_ENV1_ATK   0
#define K_ENV1_DCY   1
#define K_ENV1_SUS   2
#define K_ENV1_RLS   3
#define K_ENV1_AMT   4

#define M_ENV2       9
#define K_ENV2_ATK   0
#define K_ENV2_DCY   1
#define K_ENV2_SUS   2
#define K_ENV2_RLS   3
#define K_ENV2_AMT   4

#define M_ENV3      10
#define K_ENV3_ATK   0
#define K_ENV3_DCY   1
#define K_ENV3_SUS   2
#define K_ENV3_RLS   3
#define K_ENV3_VOL   4

#define M_NONE    0xFF
#define K_NONE    0xFF

// Selected destination indices
#define DEST_LFO1_OSC1_PIT 4
#define DEST_LFO2_OFF      0
#define DEST_ENV1_OFF      0

typedef struct knob_state knob_state;
typedef struct module_state module_state;
typedef struct synth_state synth_state;

typedef void knob_button_handler(knob_state *);
typedef float knob_mapper(knob_state *, uint8_t CC_msb, uint8_t CC_lsb);
typedef bool mod_active_predicate(const synth_state *, const module_state *);

typedef enum __attribute__((packed)) module_config_flags {
    MCF_NONE = 0,
    MCF_CTLRS = 1 << 0,
} module_config_flags;

typedef enum __attribute__((packed)) knob_config_flags {
    KCF_NONE = 0,
    MCF_PITCH = 1,
} knob_config_flags;

typedef struct choice_config {
    const char   *cc_name;
    uint8_t       cc_CC;        // MIDI CC number
    uint8_t       cc_count;     // number of choices
} choice_config;

typedef struct knob_config {
    const char   *kc_name;
    knob_config_flags kc_flags;
    uint8_t       kc_CC_msb;    // MIDI CC number, MSB
    uint8_t       kc_CC_lsb;    // MIDI CC number, LSB (zero if none)
    uint8_t       kc_LED;       // status LED number
    bool          kc_has_button; // true if knob has button
    knob_button_handler
                 *kc_button_handler; // handler for button press events
    knob_mapper  *kc_map_func;  // map MIDI CC values to user values
} knob_config;

typedef struct assign_dest {
    uint8_t       ad_module;    // index of destination module
    uint8_t       ad_control;   // index of destination control
    uint8_t       ad_CC_val;    // MIDI CC value
} assign_dest;

typedef struct assign_config {
    const char   *ac_name;
    uint8_t       ac_CC;        // MIDI CC number
    size_t        ac_dest_count;
    assign_dest const *ac_dests; // possible destinations
} assign_config;

typedef struct module_config {
    const char   *mc_name;
    module_config_flags mc_flags;
    uint8_t       mc_SPI_group;
    uint8_t       mc_SPI_bus;
    uint8_t       mc_SYSEX_addr;
    uint8_t       mc_LED;
    uint8_t       mc_color[3];
    bool          mc_has_choice;
    bool          mc_has_assign;
    size_t        mc_knob_count;
    choice_config mc_choice;
    knob_config   mc_knobs[MAX_KNOBS];
    assign_config mc_assign;
    mod_active_predicate *mc_is_active;
} module_config;

typedef struct synth_config {
    uint8_t       sc_SYSEX_addr;
    module_config sc_modules[MODULE_COUNT];
} synth_config;

extern const synth_config sc;

extern void verify_config(void);

#endif /* !CONFIG_included */
