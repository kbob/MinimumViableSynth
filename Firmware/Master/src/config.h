#ifndef CONFIG_included
#define CONFIG_included

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_DESTINATIONS 25
#define MAX_KNOBS         6
#define MODULE_COUNT     11

typedef struct knob_state knob_state;
typedef struct module_state module_state;
typedef struct synth_state synth_state;

typedef void knob_button_handler(knob_state *);
typedef float knob_mapper(knob_state *, uint8_t CC_msb, uint8_t CC_lsb);
typedef bool mod_active_predicate(const synth_state *, const module_state *);

typedef struct choice_config {
    const char   *cc_name;
    uint8_t       cc_CC;        // MIDI CC number
    uint8_t       cc_count;     // number of choices
} choice_config;

typedef struct knob_config {
    const char   *kc_name;
    uint8_t       kc_CC_msb;    // MIDI CC number, MSB
    uint8_t       kc_CC_lsb;    // MIDI CC number, LSB (zero if none)
    uint8_t       kc_LED;       // status LED number
    bool          kc_has_button; // true if knob has button
    knob_button_handler
                 *kc_button_pressed; // handler for button press events
    knob_mapper  *kc_map_func;  // map MIDI CC values to user values
    
} knob_config;

typedef struct assign_dest {
    uint8_t       ad_module;    // index of destination module
    uint8_t       ad_control;   // index of destination control
    uint8_t       ad_CC_val;    // MIDI CC value
} assign_dest;

typedef struct assign_config {
    uint8_t       ac_CC;        // MIDI CC number
    size_t        ac_dest_count;
    assign_dest const *ac_dest; // possible destinations
} assign_config;

typedef struct module_config {
    const char   *mc_name;
    uint8_t       mc_SPI_group;
    uint8_t       mc_SPI_bus;
    uint8_t       mc_SYSEX_addr;
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
    uint8_t       sc_SYSEX_address;
    module_config sc_modules[MODULE_COUNT];
} synth_config;

extern const synth_config sc;

#endif /* !CONFIG_included */
