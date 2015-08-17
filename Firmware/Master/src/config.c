#include "config.h"

// Module Colors
#define AMBER  { 0xFF, 0x99, 0x33 }
#define BLUE   { 0x4D, 0x4D, 0xFF }
#define GREEN  { 0x00, 0xFF, 0x00 }
#define RED    { 0xFF, 0x33, 0x33 }
#define YELLOW { 0xFF, 0xFF, 0x00 }

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

static const assign_dest LFO1_dests[] = {
    { .ad_module = M_LFO2, .ad_control = K_LFO2_SPD, .ad_CC_val =  0 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_AMT, .ad_CC_val =  1 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  2 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  3 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  4 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  5 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  6 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val =  7 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val =  8 },
    { .ad_module = M_FILT, .ad_control = K_FILT_FRQ, .ad_CC_val =  9 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 10 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 11 },
    { .ad_module = M_ENV3, .ad_control = K_ENV3_VOL, .ad_CC_val = 12 },
};

static const assign_dest LFO2_dests[] = {
    { .ad_module = M_LFO1, .ad_control = K_LFO1_SPD, .ad_CC_val =  0 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_AMT, .ad_CC_val =  1 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  2 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  3 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  4 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  5 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  6 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val =  7 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val =  8 },
    { .ad_module = M_FILT, .ad_control = K_FILT_FRQ, .ad_CC_val =  9 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 10 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 11 },
    { .ad_module = M_ENV3, .ad_control = K_ENV3_VOL, .ad_CC_val = 12 },
};

static const assign_dest CTLR_dests[] = {
    { .ad_module = M_LFO1, .ad_control = K_LFO1_SPD, .ad_CC_val =  0 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_AMT, .ad_CC_val =  1 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_SPD, .ad_CC_val =  2 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_AMT, .ad_CC_val =  3 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  4 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  5 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  6 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  7 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  8 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val =  9 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val = 10 },
    { .ad_module = M_FILT, .ad_control = K_FILT_FRQ, .ad_CC_val = 11 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 12 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 13 },
    { .ad_module = M_ENV3, .ad_control = K_ENV3_VOL, .ad_CC_val = 14 },
};

static const assign_dest ENV1_dests[] = {
    { .ad_module = M_LFO1, .ad_control = K_LFO1_SPD, .ad_CC_val =  0 },
    { .ad_module = M_LFO1, .ad_control = K_LFO1_AMT, .ad_CC_val =  1 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_SPD, .ad_CC_val =  2 },
    { .ad_module = M_LFO2, .ad_control = K_LFO2_AMT, .ad_CC_val =  3 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_WID, .ad_CC_val =  4 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_PIT, .ad_CC_val =  5 },
    { .ad_module = M_OSC1, .ad_control = K_OSC1_AMT, .ad_CC_val =  6 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_WID, .ad_CC_val =  7 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_PIT, .ad_CC_val =  8 },
    { .ad_module = M_OSC2, .ad_control = K_OSC2_AMT, .ad_CC_val =  9 },
    { .ad_module = M_NOIS, .ad_control = K_NOIS_AMT, .ad_CC_val = 10 },
    { .ad_module = M_FILT, .ad_control = K_FILT_RES, .ad_CC_val = 12 },
    { .ad_module = M_FILT, .ad_control = K_FILT_DRV, .ad_CC_val = 13 },
};

const synth_config sc = {
    .sc_SYSEX_address = 0,
    .sc_modules = {
        [M_LFO1] = {
            .mc_name = "LFO 1",
            .mc_SPI_group = 0,
            .mc_SPI_bus = 1,
            .mc_SYSEX_addr = 1,
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
                    .kc_CC_msb = 55,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_LFO1_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 14,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_CC = 104,
                .ac_dest_count = sizeof LFO1_dests / sizeof LFO1_dests[0],
                .ac_dest = LFO1_dests,
            },
            .mc_is_active = NULL,
        },

        [M_LFO2] = {
            .mc_name = "LFO 2",
            .mc_SPI_group = 0,
            .mc_SPI_bus = 3,
            .mc_SYSEX_addr = 2,
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
                    .kc_CC_msb = 56,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_LFO2_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 15,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_CC = 106,
                .ac_dest_count = sizeof LFO2_dests / sizeof LFO2_dests[0],
                .ac_dest = LFO2_dests,
            },
            .mc_is_active = NULL,
        },

        [M_CTLRS] = {
            .mc_name = "Controllers",
            .mc_SPI_group = 0,
            .mc_SPI_bus = 4,
            .mc_SYSEX_addr = 3,
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
                // N.B., [0] is uninitialized.
                [K_CTLRS_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 15,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_CC = 106,
                .ac_dest_count = sizeof CTLR_dests / sizeof CTLR_dests[0],
                .ac_dest = CTLR_dests,
            },
            .mc_is_active = NULL,
        },

        [M_OSC1] = {
            .mc_name = "Oscillator 1",
            .mc_SPI_group = 0,
            .mc_SPI_bus = 5,
            .mc_SYSEX_addr = 4,
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
                    .kc_CC_msb = 23,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_OSC1_PIT] = {
                    .kc_name = "Pitch",
                    .kc_CC_msb = 0, // XXX assign a CC.
                    .kc_CC_lsb = 0, // XXX assign a CC.
                    .kc_LED = 2,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL, // XXX create callback
                    .kc_map_func = NULL,
                },
                [K_OSC1_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 20,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_OSC2] = {
            .mc_name = "Oscillator 2",
            .mc_SPI_group = 1,
            .mc_SPI_bus = 1,
            .mc_SYSEX_addr = 5,
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
                    .kc_CC_msb = 23,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_OSC2_PIT] = {
                    .kc_name = "Pitch",
                    .kc_CC_msb = 25,
                    .kc_CC_lsb = 26,
                    .kc_LED = 2,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL, // XXX create callback
                    .kc_map_func = NULL,
                },
                [K_OSC2_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 21,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_NOIS] = {
            .mc_name = "Noise Source",
            .mc_SPI_group = 1,
            .mc_SPI_bus = 3,
            .mc_SYSEX_addr = 6,
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
                    .kc_CC_msb = 22,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_MIX] = {
            .mc_name = "Mixer",
            .mc_SPI_group = 1,
            .mc_SPI_bus = 4,
            .mc_SYSEX_addr = 7,
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
            .mc_SPI_group = 1,
            .mc_SPI_bus = 5,
            .mc_SYSEX_addr = 8,
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
                    .kc_CC_msb = 17,
                    .kc_CC_lsb = 49,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_FILT_RES] = {
                    .kc_name = "Resonance",
                    .kc_CC_msb = 18,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_FILT_DRV] = {
                    .kc_name = "Drive",
                    .kc_CC_msb = 19,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_FILT_KTK] = {
                    .kc_name = "Key Track",
                    .kc_CC_msb = 58,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_ENV1] = {
            .mc_name = "Envelope 1",
            .mc_SPI_group = 2,
            .mc_SPI_bus = 1,
            .mc_SYSEX_addr = 9,
            .mc_color = YELLOW,
            .mc_has_choice = false,
            .mc_has_assign = true,
            .mc_knob_count = 5,
            .mc_knobs = {
                [K_ENV1_ATK] = {
                    .kc_name = "Attack",
                    .kc_CC_msb = 59,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_DCY] = {
                    .kc_name = "Decay",
                    .kc_CC_msb = 60,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_SUS] = {
                    .kc_name = "Sustain",
                    .kc_CC_msb = 61,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_RLS] = {
                    .kc_name = "Release",
                    .kc_CC_msb = 62,
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV1_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 5,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_assign = {
                .ac_CC = 112,
                .ac_dest_count = sizeof ENV1_dests / sizeof ENV1_dests[0],
                .ac_dest = ENV1_dests,
            },
            .mc_is_active = NULL,
        },

        [M_ENV2] = {
            .mc_name = "Filter Envelope",
            .mc_SPI_group = 2,
            .mc_SPI_bus = 3,
            .mc_SYSEX_addr = 10,
            .mc_color = YELLOW,
            .mc_has_choice = false,
            .mc_has_assign = false,
            .mc_knob_count = 5,
            .mc_knobs = {
                [K_ENV2_ATK] = {
                    .kc_name = "Attack",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_DCY] = {
                    .kc_name = "Decay",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_SUS] = {
                    .kc_name = "Sustain",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_RLS] = {
                    .kc_name = "Release",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV2_AMT] = {
                    .kc_name = "Amount",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 5,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },

        [M_ENV3] = {
            .mc_name = "Amplitude Envelope",
            .mc_SPI_group = 2,
            .mc_SPI_bus = 5,
            .mc_SYSEX_addr = 11,
            .mc_color = YELLOW,
            .mc_has_choice = false,
            .mc_has_assign = false,
            .mc_knob_count = 5,
            .mc_knobs = {
                [K_ENV3_ATK] = {
                    .kc_name = "Attack",
                    .kc_CC_msb = 27,
                    .kc_CC_lsb = 0,
                    .kc_LED = 1,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_DCY] = {
                    .kc_name = "Decay",
                    .kc_CC_msb = 28,
                    .kc_CC_lsb = 0,
                    .kc_LED = 2,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_SUS] = {
                    .kc_name = "Sustain",
                    .kc_CC_msb = 29,
                    .kc_CC_lsb = 0,
                    .kc_LED = 3,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_RLS] = {
                    .kc_name = "Release",
                    .kc_CC_msb = 30,
                    .kc_CC_lsb = 0,
                    .kc_LED = 4,
                    .kc_has_button = false,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
                [K_ENV3_VOL] = {
                    .kc_name = "Master Volume",
                    .kc_CC_msb = 0, // XXX assign a CC
                    .kc_CC_lsb = 0,
                    .kc_LED = 5,
                    .kc_has_button = true,
                    .kc_button_pressed = NULL,
                    .kc_map_func = NULL,
                },
            },
            .mc_is_active = NULL,
        },
    },
};
