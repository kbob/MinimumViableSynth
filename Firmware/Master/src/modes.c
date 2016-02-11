#include "modes.h"

#include <assert.h>

#include "config.h"
#include "state.h"
#include "systick.h"

typedef enum AssignmentState {
    AS_INACTIVE,
    AS_ACTIVE,
    AS_CONFIRMING,          // when the confirming animation runs.
} AssignmentState;

static AssignmentState a_state;

static uint32_t a_start_time;
static uint32_t a_confirm_time;
static size_t   a_src_mod_index      = M_NONE;
static size_t   a_dest_mod_index     = M_NONE;
static size_t   a_dest_knob_index    = K_NONE;
static size_t   current_module_index = M_NONE;
static size_t   current_knob_index   = K_NONE;

bool module_is_current(size_t module_index)
{
    return module_index == current_module_index;
}

bool module_is_active(size_t module_index)
{
    return true;                // XXX do this right.
}

bool knob_is_current(size_t module_index, size_t knob_index)
{
    return (module_index == current_module_index &&
            knob_index == current_knob_index);
}

bool knob_is_working(size_t module_index, size_t knob_index)
{
    const knob_state *ks = &ss.ss_modules[module_index].ms_knobs[knob_index];
    return ks->ks_should_export;
}

bool module_is_active_assign_source(size_t module_index)
{
    return module_index == a_src_mod_index;
}

bool knob_is_active_assign_dest(size_t module_index, size_t knob_index)
{
    return module_index == a_dest_mod_index && knob_index == a_dest_knob_index;
}

bool knob_is_assign_dest(size_t module_index, size_t knob_index)
{
    if (a_src_mod_index == M_NONE)
        return false;
    size_t dest_idx = dest_index_by_knob(a_src_mod_index,
                                         module_index, knob_index);
    return dest_idx != D_NONE;
}

uint32_t assignment_start_time(void)
{
    return a_start_time;
}

size_t active_source_mod_index(void)
{
    return a_src_mod_index;
}

size_t active_dest_mod_index(void)
{
    return a_dest_mod_index;
}

size_t active_dest_knob_index(void)
{
    return a_dest_knob_index;
}

void begin_assignment(size_t src_mod_index)
{
    module_config const *src_mc     = &sc.sc_modules[src_mod_index];
    module_state  const *src_ms     = &ss.ss_modules[src_mod_index];    assign_config const *ac         = &src_mc->mc_assign;
    assign_dest   const *dest       = &ac->ac_dests[src_ms->ms_assign.as_index];
    assert(src_mc->mc_has_assign);
    a_state = AS_ACTIVE;
    a_start_time = system_millis;
    a_confirm_time = 0;
    a_src_mod_index = src_mod_index;
    a_dest_mod_index = dest->ad_module;
    a_dest_knob_index = dest->ad_control;
}

void confirm_assignment(size_t dest_mod_index, size_t dest_knob_index)
{
    a_state = AS_CONFIRMING;
    a_start_time = 0;
    a_confirm_time = system_millis;
    a_dest_mod_index = dest_mod_index;
    a_dest_knob_index = dest_knob_index;
}

void cancel_assignment(void)
{
    a_state = AS_INACTIVE;
    a_start_time = 0;
    a_confirm_time = 0;
    a_src_mod_index = M_NONE;
    a_dest_mod_index = M_NONE;
    a_dest_knob_index = K_NONE;
}

bool source_is_assigned(size_t module_index)
{
    module_config const *mc = &sc.sc_modules[module_index];
    module_state  const *ms = &ss.ss_modules[module_index];
    assign_config const *ac = &mc->mc_assign;
    assign_state  const *as = &ms->ms_assign;
    assert(mc->mc_has_assign);
    assert(as->as_index < ac->ac_dest_count);
    return ac->ac_dests[as->as_index].ad_module != M_NONE;
}

bool assignment_is_active(void)
{
    if (a_state == AS_ACTIVE &&
        system_millis - a_start_time > ASSIGN_TIMEOUT_MSEC)
    {
        cancel_assignment();
    }
    return a_state == AS_ACTIVE;
}

bool assignment_is_confirmed(void)
{
    if (a_state == AS_CONFIRMING &&
        system_millis - a_confirm_time > CONFIRM_TIMEOUT_MSEC)
    {
        cancel_assignment();
    }
    return a_state == AS_CONFIRMING;
}

void set_current_knob(size_t module_index, size_t knob_index)
{
    current_module_index = module_index;
    current_knob_index = knob_index;
}
