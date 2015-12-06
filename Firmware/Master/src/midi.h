#ifndef MIDI_included
#define MIDI_included

#include <stddef.h>
#include <stdint.h>

// MIDI - plumbing

typedef void MIDI_handler(uint8_t *pkt, size_t size, void *user_data);

extern void MIDI_setup(void);

// copies message to internal queue.
// returns 0 on success or an error code
extern int MIDI_send_message(uint8_t *pkt, size_t size);

// returns the previous handler or NULL if none.
extern MIDI_handler *MIDI_register_handler(MIDI_handler *cb);


// MIDI - porcelain

// MIDI_send functions for all message types

extern void MIDI_send_note_off             (uint8_t channel,
                                            uint8_t note,
                                            uint8_t velocity);

extern void MIDI_send_note_on              (uint8_t channel,
                                            uint8_t note,
                                            uint8_t velocity);

extern void MIDI_send_poly_pressure        (uint8_t channel,
                                            uint8_t note,
                                            uint8_t pressure);

extern void MIDI_send_control_change       (uint8_t channel,
                                            uint8_t control,
                                            uint8_t value);

extern void MIDI_send_program_change       (uint8_t channel,
                                            uint8_t patch);

extern void MIDI_send_channel_pressure     (uint8_t channel,
                                            uint8_t value);

extern void MIDI_send_pitch_bend           (uint8_t channel,
                                            uint16_t value);

// SYSEX message must include leading 0xF0 and trailing 0xF7 bytes.
extern void MIDI_send_raw_SYSEX            (uint8_t const *message,
                                            size_t size);

extern void MIDI_send_MTC_quarter          (uint8_t type,
                                            uint8_t values);

extern void MIDI_send_song_position        (uint16_t beat);

extern void MIDI_send_song_select          (uint8_t song);

extern void MIDI_send_tune_request         (void);

extern void MIDI_send_timing_clock         (void);

extern void MIDI_send_start                (void);

extern void MIDI_send_continue             (void);

extern void MIDI_send_stop                 (void);

extern void MIDI_send_active_sensing       (void);

extern void MIDI_send_reset                (void);


// Handlers for incoming MIDI messages

#define DEFINE_MESSAGE_HANDLER(type, args)                      \
                                                                \
    typedef void MIDI_##type##_handler args;                    \
                                                                \
    extern MIDI_##type##_handler *                              \
    MIDI_register_##type##_handler(MIDI_##type##_handler);

DEFINE_MESSAGE_HANDLER(note_off,
                       (uint8_t channel, uint8_t note, uint8_t velocity));

DEFINE_MESSAGE_HANDLER(note_on,
                       (uint8_t channel, uint8_t note, uint8_t velocity));

DEFINE_MESSAGE_HANDLER(poly_pressure,
                       (uint8_t channel, uint8_t note, uint8_t pressure));

DEFINE_MESSAGE_HANDLER(control_change,
                       (uint8_t channel, uint8_t control, uint8_t value));

DEFINE_MESSAGE_HANDLER(program_change, (uint8_t channel, uint8_t patch));

DEFINE_MESSAGE_HANDLER(channel_pressure, (uint8_t channel, uint8_t value));

DEFINE_MESSAGE_HANDLER(pitch_bend, (uint8_t channel, uint16_t value));

DEFINE_MESSAGE_HANDLER(SYSEX, (uint8_t const *message, size_t size));

DEFINE_MESSAGE_HANDLER(MTC_quarter_frame, (void));

DEFINE_MESSAGE_HANDLER(song_position, (uint8_t type, uint8_t values));

DEFINE_MESSAGE_HANDLER(song_select, (uint16_t beat));

DEFINE_MESSAGE_HANDLER(tune_request, (uint8_t song));

DEFINE_MESSAGE_HANDLER(timing_clock, (void));

DEFINE_MESSAGE_HANDLER(start, (void));

DEFINE_MESSAGE_HANDLER(continue, (void));

DEFINE_MESSAGE_HANDLER(stop, (void));

DEFINE_MESSAGE_HANDLER(active_sensing, (void));

DEFINE_MESSAGE_HANDLER(reset, (void));

#endif /* !MIDI_included */
