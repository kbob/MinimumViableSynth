#ifndef SPI_RESPONDER_included
#define SPI_RESPONDER_included

#include <stdbool.h>
#include <stddef.h>

extern void SPI_responder_setup(void);

// Query interface for animation

extern bool module_is_current(size_t module_index);
extern bool module_is_active(size_t module_index);
extern bool knob_is_current(size_t module_index, size_t knob_index);
extern bool knob_is_working(size_t module_index, size_t knob_index);

extern bool source_is_assigned(size_t module_index);

extern bool assignment_is_active(void);
extern bool assignment_is_confirmed(void);
extern bool module_is_active_assign_source(size_t module_index);
extern bool knob_is_active_assign_dest(size_t module_index, size_t knob_index);
extern bool knob_is_assign_dest(size_t module_index, size_t knob_index);

#endif /* !SPI_RESPONDER_included */
