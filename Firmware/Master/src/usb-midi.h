#ifndef USB_MIDI_included
#define USB_MIDI_included

#include <stdbool.h>            // XXX
#include <stddef.h>
#include <stdint.h>

typedef void usb_midi_packet_handler(uint8_t *pkt, size_t size, void *user_data);

extern void usb_midi_setup(void);

extern void usb_midi_poll(void);

extern void usb_midi_send_message(uint8_t const *msg, size_t size);

extern void usb_midi_report_and_clear_stats(void);

#endif /* !USB_MIDI_included */
