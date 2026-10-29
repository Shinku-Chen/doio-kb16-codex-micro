#include QMK_KEYBOARD_H

#include "poc_config.h"
#include "poc_mode.h"
#include "poc_via.h"
#include "codex_micro_protocol.h"

#ifdef POC_MODE_SWITCH

void poc_via_init(void) {}

uint8_t poc_via_layer_count(void) {
    return POC_VIA_LAYER_COUNT;
}

uint8_t poc_via_layer_offset(void) {
    return POC_VIA_LAYER_OFFSET;
}

bool poc_via_layer_allowed(uint8_t via_layer) {
    return via_layer < POC_VIA_LAYER_COUNT;
}

// QMK VIA hook: any command from VIA counts as "VIA is alive".
// Return false so QMK's VIA stack still handles the command.
bool via_command_kb(uint8_t *data, uint8_t length) {
    // Codex Channel 2/3 reports use report ID 6 and 64 bytes. Route them
    // before VIA sees them; VIA's own reports are 32 bytes.
    if (length == CODEX_MICRO_REPORT_SIZE && data[0] == CODEX_MICRO_REPORT_ID) {
        codex_micro_raw_hid_receive(data, length);
        return true;
    }

    poc_mode_note_via_command();
    return false;
}

#endif
