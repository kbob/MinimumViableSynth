#ifndef MODES_included
#define MODES_included

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ASSIGN_TIMEOUT_MSEC  2500
#define CONFIRM_TIMEOUT_MSEC  500


// Query current/active modules and current/working knobs

extern bool module_is_current(size_t module_index);
extern bool module_is_active(size_t module_index);
extern bool knob_is_current(size_t module_index, size_t knob_index);
extern bool knob_is_working(size_t module_index, size_t knob_index);


// Query assignment state

extern bool source_is_assigned(size_t module_index);

extern bool assignment_is_active(void);
extern bool assignment_is_confirmed(void);
extern bool module_is_active_assign_source(size_t module_index);
extern bool knob_is_active_assign_dest(size_t module_index, size_t knob_index);
extern bool knob_is_assign_dest(size_t module_index, size_t knob_index);
extern uint32_t assignment_start_time(void);

extern size_t active_source_mod_index(void);
extern size_t active_dest_mod_index(void);
extern size_t active_dest_knob_index(void);


// Assignment mode control

extern void begin_assignment(size_t src_mod_index);
extern void confirm_assignment(size_t dest_mod_index, size_t dest_knob_index);
extern void cancel_assignment(void);


// Current module/knob control

extern void set_current_knob(size_t module_index, size_t knob_index);

#endif /* !MODES_included */
