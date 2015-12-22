#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Module Colors
#define AMBER  { 0xFF, 0x77, 0x00 }
#define BLUE   { 0x00, 0x00, 0xFF }
#define GREEN  { 0x00, 0xFF, 0x00 }
#define RED    { 0xFF, 0x33, 0x33 }
#define YELLOW { 0xFF, 0xFF, 0x00 }
#define LIGHT_BLUE {0x44, 0x44, 0xFF }
#define WHITE  { 0xFF, 0xFF, 0xFF }

static const assign_dest LFO1_dests[] = {
    { .ad_module = M_NONE, .ad_control = K_NONE,     .ad_CC_val = 12 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_SPD, .ad_CC_val = 12 }, // XXX
    { .ad_module = M_LFO2, .ad_control = K_LFO2_AMT, .ad_CC_val = 12 }, // XXX
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  1 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  0 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  2 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  4 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  3 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val =  5 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val =  6 },
    { .ad_module = M_FILT, .ad_control = K_FILT_FRQ, .ad_CC_val =  7 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val =  8 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val =  9 },
    { .ad_module = M_ENV3, .ad_control = K_ENV3_VOL, .ad_CC_val = 10 },
};

static const assign_dest LFO2_dests[] = {
    { .ad_module = M_NONE, .ad_control = K_NONE,     .ad_CC_val =  0 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_SPD, .ad_CC_val =  1 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_AMT, .ad_CC_val =  2 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  3 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  4 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  5 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  6 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  7 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val =  8 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val =  9 },
    { .ad_module = M_FILT, .ad_control = K_FILT_FRQ, .ad_CC_val = 10 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 11 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 12 },
    { .ad_module = M_ENV3, .ad_control = K_ENV3_VOL, .ad_CC_val = 13 },
};

static const assign_dest CTLR_dests[] = {
    { .ad_module = M_NONE, .ad_control = K_NONE,     .ad_CC_val =  0 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_SPD, .ad_CC_val =  1 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_AMT, .ad_CC_val =  2 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_SPD, .ad_CC_val =  3 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_AMT, .ad_CC_val =  4 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  5 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  6 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  7 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  8 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  9 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val = 10 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val = 11 },
    { .ad_module = M_FILT, .ad_control = K_FILT_FRQ, .ad_CC_val = 12 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 13 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 14 },
    { .ad_module = M_ENV3, .ad_control = K_ENV3_VOL, .ad_CC_val = 15 },
};

static const assign_dest ENV1_dests[] = {
    { .ad_module = M_NONE, .ad_control = K_NONE,     .ad_CC_val =  0 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_SPD, .ad_CC_val =  1 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_AMT, .ad_CC_val =  2 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_SPD, .ad_CC_val =  3 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_AMT, .ad_CC_val =  4 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  5 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  6 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  7 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  8 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  9 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val = 10 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val = 11 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 12 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 13 },
};

const synth_config sc = {
    .sc_SYSEX_addr = 0,
    .sc_modules = {
        [M_LFO1] = {
            .mc_name = "LFO 1",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 0,
            .mc_SPI_bus = 1,
            .mc_SYSEX_addr = 1,
            .mc_LED = 0,
            .mc_color = BLUE,
            .mc_has_choice = true,
            .mc_has_assign = true,
            .mc_knob_count = 2,
            .mc_choice = {
                .cc_name = "Waveform",
                .cc_CC = 103,
                .cc_count = 6,
            },
            .mc_knobs = {
                [K_LFO1_SPD] = {
                    .kc_name = "Speed",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 55,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = BLUE,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_LFO1_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 14,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_color = BLUE,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_name = "Assign",
                .ac_CC = 104,
                .ac_dest_count = (&LFO1_dests)[1] - LFO1_dests,
                .ac_dests = LFO1_dests,
            },
            .mc_is_active = NULL,
        },

        [M_LFO2] = {
            .mc_name = "LFO 2",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 0,
            .mc_SPI_bus = 3,
            .mc_SYSEX_addr = 2,
            .mc_LED = 0,
            .mc_color = BLUE,
            .mc_has_choice = true,
            .mc_has_assign = true,
            .mc_knob_count = 2,
            .mc_choice = {
                .cc_name = "Waveform",
                .cc_CC = 105,
                .cc_count = 6,
            },
            .mc_knobs = {
                [K_LFO2_SPD] = {
                    .kc_name = "Speed",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 56,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = BLUE,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_LFO2_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 15,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_color = BLUE,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_name = "Assign",
                .ac_CC = 106,
                .ac_dest_count = (&LFO2_dests)[1] - LFO2_dests,
                .ac_dests = LFO2_dests,
            },
            .mc_is_active = NULL,
        },

        [M_CTLRS] = {
            .mc_name = "Controllers",
            .mc_flags = MCF_CTLRS,
            .mc_SPI_group = 0,
            .mc_SPI_bus = 4,
            .mc_SYSEX_addr = 3,
            .mc_LED = 0,
            .mc_color = BLUE,
            .mc_has_choice = true,
            .mc_has_assign = true,
            .mc_knob_count = 2,
            .mc_choice = {
                .cc_name = "Source",
                .cc_CC = 0,     // XXX assign a CC.
                .cc_count = 4,
            },
            .mc_knobs = {
                // N.B., [0] is uninitialized.
                [K_CTLRS_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC.
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = BLUE,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_name = "Assign",
                .ac_CC = 102,
                .ac_dest_count = (&CTLR_dests)[1] - CTLR_dests,
                .ac_dests = CTLR_dests,
            },
            .mc_is_active = NULL,
        },

        [M_OSC1] = {
            .mc_name = "Oscillator 1",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 0,
            .mc_SPI_bus = 5,
            .mc_SYSEX_addr = 4,
            .mc_LED = 0,
            .mc_color = AMBER,
            .mc_has_choice = true,
            .mc_has_assign = false,
            .mc_knob_count = 3,
            .mc_choice = {
                .cc_name = "Waveform",
                .cc_CC = 108,
                .cc_count = 4,
            },
            .mc_knobs = {
                [K_OSC1_WID] = {
                    .kc_name = "Width",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 23,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_OSC1_PIT] = {
                    .kc_name = "Pitch",
                    .kc_flags = KCF_PITCH,
                    .kc_CC_msb = 0, // XXX assign a CC.
                    .kc_CC_lsb = 0, // XXX assign a CC.
                    .kc_LED = 2,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL, // XXX create callback
                    .kc_map_func = NULL,
                },
                [K_OSC1_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 20,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_OSC2] = {
            .mc_name = "Oscillator 2",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 1,
            .mc_SPI_bus = 1,
            .mc_SYSEX_addr = 5,
            .mc_LED = 0,
            .mc_color = AMBER,
            .mc_has_choice = true,
            .mc_has_assign = false,
            .mc_knob_count = 3,
            .mc_choice = {
                .cc_name = "Waveform",
                .cc_CC = 109,
                .cc_count = 4,
            },
            .mc_knobs = {
                [K_OSC2_WID] = {
                    .kc_name = "Width",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 24,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_OSC2_PIT] = {
                    .kc_name = "Pitch",
                    .kc_flags = KCF_PITCH,
                    .kc_CC_msb = 25,
                    .kc_CC_lsb = 26,
                    .kc_LED = 2,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL, // XXX create callback
                    .kc_map_func = NULL,
                },
                [K_OSC2_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 21,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_NOIS] = {
            .mc_name = "Noise Source",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 1,
            .mc_SPI_bus = 3,
            .mc_SYSEX_addr = 6,
            .mc_LED = 0,
            .mc_color = AMBER,
            .mc_has_choice = true,
            .mc_has_assign = false,
            .mc_knob_count = 1,
            .mc_choice = {
                .cc_name = "Spectrum",
                .cc_CC = 110,
                .cc_count = 3,
            },
            .mc_knobs = {
                [K_NOIS_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 22,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = AMBER,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_MIX] = {
            .mc_name = "Mixer",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 1,
            .mc_SPI_bus = 4,
            .mc_SYSEX_addr = 7,
            .mc_LED = 0,
            .mc_color = GREEN,
            .mc_has_choice = true,
            .mc_has_assign = false,
            .mc_knob_count = 0,
            .mc_choice = {
                .cc_name = "Operator",
                .cc_CC = 107,
                .cc_count = 3,
            },
            .mc_is_active = NULL,
        },

        [M_FILT] = {
            .mc_name = "Filter",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 1,
            .mc_SPI_bus = 5,
            .mc_SYSEX_addr = 8,
            .mc_LED = 0,
            .mc_color = RED,
            .mc_has_choice = true,
            .mc_has_assign = false,
            .mc_knob_count = 4,
            .mc_choice = {
                .cc_name = "Type",
                .cc_CC = 111,
                .cc_count = 6,
            },
            .mc_knobs = {
               [K_FILT_FRQ] = {
                    .kc_name = "Cutoff",
                    .kc_flags = KCF_FRQ,
                    .kc_CC_msb = 17,
                    .kc_CC_lsb = 49,
                    .kc_LED = 1,
                    .kc_color = RED,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_FILT_RES] = {
                    .kc_name = "Resonance",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 18,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_color = RED,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_FILT_DRV] = {
                    .kc_name = "Drive",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 19,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_color = RED,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_FILT_KTK] = {
                    .kc_name = "Key Track",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 58,
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_color = RED,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_ENV1] = {
            .mc_name = "Envelope 1",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 2,
            .mc_SPI_bus = 1,
            .mc_SYSEX_addr = 9,
            .mc_LED = 0,
            .mc_color = YELLOW,
            .mc_has_choice = false,
            .mc_has_assign = true,
            .mc_knob_count = 5,
            .mc_knobs = {
                [K_ENV1_ATK] = {
                    .kc_name = "Attack",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 59,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_DCY] = {
                    .kc_name = "Decay",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 60,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_SUS] = {
                    .kc_name = "Sustain",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 61,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_RLS] = {
                    .kc_name = "Release",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 62,
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 5,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_name = "Assign",
                .ac_CC = 112,
                .ac_dest_count = (&ENV1_dests)[1] - ENV1_dests,
                .ac_dests = ENV1_dests,
            },
            .mc_is_active = NULL,
        },

        [M_ENV2] = {
            .mc_name = "Filter Envelope",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 2,
            .mc_SPI_bus = 3,
            .mc_SYSEX_addr = 10,
            .mc_LED = 0,
            .mc_color = YELLOW,
            .mc_has_choice = false,
            .mc_has_assign = false,
            .mc_knob_count = 5,
            .mc_knobs = {
                [K_ENV2_ATK] = {
                    .kc_name = "Attack",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_DCY] = {
                    .kc_name = "Decay",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_SUS] = {
                    .kc_name = "Sustain",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_RLS] = {
                    .kc_name = "Release",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_AMT] = {
                    .kc_name = "Amount",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 5,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_ENV3] = {
            .mc_name = "Amplitude Envelope",
            .mc_flags = MCF_NONE,
            .mc_SPI_group = 2,
            .mc_SPI_bus = 5,
            .mc_SYSEX_addr = 11,
            .mc_LED = 0,
            .mc_color = YELLOW,
            .mc_has_choice = false,
            .mc_has_assign = false,
            .mc_knob_count = 5,
            .mc_knobs = {
                [K_ENV3_ATK] = {
                    .kc_name = "Attack",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 27,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_DCY] = {
                    .kc_name = "Decay",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 28,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_SUS] = {
                    .kc_name = "Sustain",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 29,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_RLS] = {
                    .kc_name = "Release",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 30,
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_color = LIGHT_BLUE,
                    .kc_has_button = false,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_VOL] = {
                    .kc_name = "Master Volume",
                    .kc_flags = KCF_NONE,
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 5,
                    .kc_color = WHITE,
                    .kc_has_button = true,
                    .kc_button_handler = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },
    },
};

size_t dest_index_by_knob(size_t src_mod_idx,
                          size_t dst_mod_idx,
                          size_t dst_knob_idx)
{
    assert(src_mod_idx < MODULE_COUNT);
    module_config const *mc = &sc.sc_modules[src_mod_idx];
    assign_config const *ac = &mc->mc_assign;
    assign_dest   const *dests = ac->ac_dests;
    size_t               ndest = ac->ac_dest_count;
    for (size_t i = 0; i < ndest; i++) {
        assign_dest const *d = &dests[i];
        if (d->ad_module == dst_mod_idx && d->ad_control == dst_knob_idx)
            return i;
    }
    return D_NONE;
}

#ifndef NDEBUG

static const char *CC_used[256][2];

static void use_CC(uint8_t cc, const char *module, const char *ctrl)
{
    if (cc == 0)
        fprintf(stderr, "WARNING: %s.%s has zero CC\n", module, ctrl);
    else {
        if (CC_used[cc][0]) {
            fprintf(stderr,
                    "ERROR: Duplicate CC %d in %s.%s and %s.%s\n",
                    cc, CC_used[cc][0], CC_used[cc][1], module, ctrl);
            assert(false);
        }
        CC_used[cc][0] = module;
        CC_used[cc][1] = ctrl;
    }
}

#define MAX_LEDS (MAX_KNOBS + 2)
typedef struct LED_mask {
    const char *lm_label;
    size_t lm_count;
    bool lm_used[MAX_LEDS];
} LED_mask;


static void begin_LED_group(LED_mask *mask, const char *label)
{
    memset(mask, 0, sizeof *mask);
    mask->lm_label = label;
}

static void use_LED(LED_mask *mask, uint8_t led)
{
    if (led >= MAX_LEDS)
        fprintf(stderr, "%s: ", mask->lm_label);
    assert(led < MAX_LEDS);
    if (mask->lm_used[led])
        fprintf(stderr, "%s: ", mask->lm_label);
    assert(!mask->lm_used[led]);
    mask->lm_used[led] = true;
    mask->lm_count++;
}

static void end_LED_group(LED_mask *mask)
{
    for (size_t i = 0; i < mask->lm_count; i++)
        assert(mask->lm_used[i]);

    for (size_t i = mask->lm_count; i < MAX_LEDS; i++)
        assert(!mask->lm_used[i]);
}

static void verify_assign_config(const assign_config *acp)
{
    assert(acp->ac_dest_count <= MAX_DESTS);

    for (size_t i = 2; i < acp->ac_dest_count; i++) {
        const assign_dest *d0p = &acp->ac_dests[i - 1];
        const assign_dest *d1p = &acp->ac_dests[i];
        uint8_t m0 = d0p->ad_module, m1 = d1p->ad_module;
        uint8_t c0 = d0p->ad_control, c1 = d1p->ad_control;
        assert(m0 < m1 || (m0 == m1 && c0 < c1));
    }
}

static void verify_module_config(const module_config *mcp)
{
    assert(mcp->mc_name);
    assert(mcp->mc_SPI_group < 3);
    uint8_t bus = mcp->mc_SPI_bus;
    assert(bus == 1 || bus == 3 || bus == 4 || bus == 5);

    if (mcp->mc_has_choice) {
        assert(mcp->mc_choice.cc_name);
        use_CC(mcp->mc_choice.cc_CC, mcp->mc_name, mcp->mc_choice.cc_name);
    } else
        assert(!mcp->mc_choice.cc_name);

    if (mcp->mc_has_assign) {
        const assign_config *acp = &mcp->mc_assign;
        assert(acp->ac_name);
        use_CC(acp->ac_CC, mcp->mc_name, acp->ac_name);
        verify_assign_config(acp);
    } else
        assert(!mcp->mc_assign.ac_name);

    assert(mcp->mc_knob_count <= MAX_KNOBS);

    for (size_t i = 0; i < mcp->mc_knob_count; i++) {
        const knob_config *kcp = &mcp->mc_knobs[i];
        if (!kcp->kc_name) {
            // The Controllers module has a missing knob.
            assert(mcp->mc_flags & MCF_CTLRS);
            continue;
        }
        use_CC(kcp->kc_CC_msb, mcp->mc_name, kcp->kc_name);
        if (kcp->kc_CC_lsb)
            use_CC(kcp->kc_CC_lsb, mcp->mc_name, kcp->kc_name);
    }

    for (size_t i = mcp->mc_knob_count; i < MAX_KNOBS; i++)
        assert(!mcp->mc_knobs[i].kc_name);

    LED_mask mask;
    begin_LED_group(&mask, mcp->mc_name);
    use_LED(&mask, mcp->mc_LED);
    for (size_t i = 0; i < mcp->mc_knob_count; i++)
        if (mcp->mc_knobs[i].kc_name)
            use_LED(&mask, mcp->mc_knobs[i].kc_LED);
    end_LED_group(&mask);
}

static void verify_synth_config(const synth_config *scp)
{
    // Verify ascending SYSEX addresses
    for (size_t i = 0; i < MODULE_COUNT; i++)
        assert(scp->sc_modules[i].mc_SYSEX_addr == i + 1);

    // Verify unique SPI addresses
    for (size_t i = 0; i < MODULE_COUNT; i++) {
        const module_config *mip = &scp->sc_modules[i];
        for (size_t j = i + 1; j < MODULE_COUNT; j++) {
            const module_config *mjp = &scp->sc_modules[j];
            assert(mip->mc_SPI_group != mjp->mc_SPI_group ||
                   mip->mc_SPI_bus != mjp->mc_SPI_bus);
        }
    }

    for (size_t i = 0; i < MODULE_COUNT; i++)
        verify_module_config(&scp->sc_modules[i]);
}

void verify_config(void)
{
    verify_synth_config(&sc);
    
    // Verify the constant, DEST_LFO1_OSC1_PIT.
    const module_config *lfo1 = &sc.sc_modules[M_LFO1];
    assert(DEST_LFO1_OSC1_PIT < lfo1->mc_assign.ac_dest_count);
    const assign_dest *adp = &lfo1->mc_assign.ac_dests[DEST_LFO1_OSC1_PIT];
    assert(adp->ad_module == M_OSC1 && adp->ad_control == K_OSC1_PIT);
}

#endif /* !NDEBUG */
