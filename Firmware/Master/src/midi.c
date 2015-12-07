#include "midi.h"

#include <assert.h>

void MIDI_setup(void)
{}

// copies message to internal queue.
// returns 0 on success or an error code
int MIDI_send_message(uint8_t *pkt, size_t size)
{
    return 0;
}

// returns the previous handler or NULL if none.
MIDI_handler *MIDI_register_handler(MIDI_handler *cb)
{
    return 0;
}


// MIDI - porcelain

// MIDI_send functions for all message types

void MIDI_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

void MIDI_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity);

void MIDI_send_poly_pressure(uint8_t channel, uint8_t note, uint8_t pressure);

void MIDI_send_control_change(uint8_t channel, uint8_t control, uint8_t value);

void MIDI_send_program_change(uint8_t channel, uint8_t patch);

void MIDI_send_channel_pressure(uint8_t channel, uint8_t value);

void MIDI_send_pitch_bend(uint8_t channel, uint16_t value);

void MIDI_send_raw_SYSEX(uint8_t const *message, size_t size)
{
    assert(size >= 2);
    assert(message[0] == 0xF0);
    assert(message[size - 1] == 0xF7);
}

void MIDI_send_MTC_quarter(uint8_t type, uint8_t values);

void MIDI_send_song_position(uint16_t beat);

void MIDI_send_song_select(uint8_t song);

void MIDI_send_tune_request(void);

void MIDI_send_timing_clock(void);

void MIDI_send_start(void);

void MIDI_send_continue(void);

void MIDI_send_stop(void);

void MIDI_send_active_sensing(void);

void MIDI_send_reset(void);


// Handlers for incoming MIDI messages

extern MIDI_note_off_handler *
MIDI_register_note_off_handler(MIDI_note_off_handler *handler)
{
    return 0;
}

extern MIDI_note_on_handler *
MIDI_register_note_on_handler(MIDI_note_on_handler *handler)
{
    return 0;
}

extern MIDI_poly_pressure_handler *
MIDI_register_poly_pressure_handler(MIDI_poly_pressure_handler *handler)
{
    return 0;
}

extern MIDI_control_change_handler *
MIDI_register_control_change_handler(MIDI_control_change_handler *handler)
{
    return 0;
}

extern MIDI_program_change_handler *
MIDI_register_program_change_handler(MIDI_program_change_handler *handler)
{
    return 0;
}

extern MIDI_channel_pressure_handler *
MIDI_register_channel_pressure_handler(MIDI_channel_pressure_handler *handler)
{
    return 0;
}

extern MIDI_pitch_bend_handler *
MIDI_register_pitch_bend_handler(MIDI_pitch_bend_handler *handler)
{
    return 0;
}

extern MIDI_SYSEX_handler *
MIDI_register_SYSEX_handler(MIDI_SYSEX_handler *handler)
{
    return 0;
}

extern MIDI_MTC_quarter_frame_handler *
MIDI_register_MTC_quarter_frame_handler(MIDI_MTC_quarter_frame_handler *handler)
{
    return 0;
}

extern MIDI_song_position_handler *
MIDI_register_song_position_handler(MIDI_song_position_handler *handler)
{
    return 0;
}

extern MIDI_song_select_handler *
MIDI_register_song_select_handler(MIDI_song_select_handler *handler)
{
    return 0;
}

extern MIDI_tune_request_handler *
MIDI_register_tune_request_handler(MIDI_tune_request_handler *handler)
{
    return 0;
}

extern MIDI_timing_clock_handler *
MIDI_register_timing_clock_handler(MIDI_timing_clock_handler *handler)
{
    return 0;
}

extern MIDI_start_handler *
MIDI_register_start_handler(MIDI_start_handler *handler)
{
    return 0;
}

extern MIDI_continue_handler *
MIDI_register_continue_handler(MIDI_continue_handler *handler)
{
    return 0;
}

extern MIDI_stop_handler *
MIDI_register_stop_handler(MIDI_stop_handler *handler)
{
    return 0;
}

extern MIDI_active_sensing_handler *
MIDI_register_active_sensing_handler(MIDI_active_sensing_handler *handler)
{
    return 0;
}

extern MIDI_reset_handler *
MIDI_register_reset_handler(MIDI_reset_handler *handler)
{
    return 0;
}
