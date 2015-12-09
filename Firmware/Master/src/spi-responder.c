#include "spi-responder.h"

#include "config.h"
#include "midi.h"
#include "spi-proto.h"
#include "state.h"

static void handle_slave_state(size_t             module_index,
                               slave_state const *sls,
                               void              *user_data)
{
    module_config const *mc = &sc.sc_modules[module_index];
    module_state        *ms = &ss.ss_modules[module_index];

    if (sls->ss_buttons & SBB_CHOICE) {
        uint8_t value = ms->ms_choice.cs_value;
        value++;
        if (value >= mc->mc_choice.cc_count)
            value = 0;
        ms->ms_choice.cs_value = value;
        MIDI_send_control_change(MIDI_default_channel,
                                 mc->mc_choice.cc_CC,
                                 value);
    }
}

void SPI_responder_setup(void)
{
    SPI_proto_register_slave_state_handler(handle_slave_state, NULL);
}
