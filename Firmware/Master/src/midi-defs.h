#ifndef MIDI_DEFS_included
#define MIDI_DEFS_included

typedef enum MIDI_status_nybble {
    NoteOff         = 0x80,
    NoteOn          = 0x90,
    PolyPress       = 0xA0,
    ControlChange   = 0xB0,
    ProgramChange   = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend       = 0xE0,
    SystemCommon    = 0xF0,
} MIDI_status_nybble;

#endif /* !MIDI_DEFS_included */
