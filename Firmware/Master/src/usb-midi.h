#ifndef USB_MIDI_included
#define USB_MIDI_included

#include <stdbool.h>

extern void usb_midi_setup(void);

extern void usb_midi_poll(void);

extern void usb_midi_send_note(bool on_off);

#endif /* !USB_MIDI_included */
