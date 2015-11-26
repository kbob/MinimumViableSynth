#ifndef ANIM_included
#define ANIM_included

#include <stddef.h>
#include <stdint.h>

typedef size_t packed_RGB;

extern packed_RGB anim_module_color(uint32_t msec, size_t module_index);

extern packed_RGB anim_knob_color(uint32_t msec,
                                  size_t   module_index,
                                  size_t   knob_index);

extern uint8_t anim_choice_brightness(uint32_t msec, size_t module_index);

extern uint8_t anim_assign_brightness(uint32_t msec, size_t module_index);

#endif /* !ANIM_included */
