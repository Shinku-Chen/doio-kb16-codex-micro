#include QMK_KEYBOARD_H

#include "codex_micro_protocol.h"
#include "poc_config.h"
#include "poc_mode.h"
#include "poc_power.h"
#include "poc_via.h"
#include "raw_hid.h"
#include "via.h"

static bool valid_via_command(const uint8_t *data, uint8_t length) {
    if (length != 32 || data[0] < id_get_protocol_version || data[0] > id_dynamic_keymap_set_encoder) {
        return false;
    }
    switch (data[0]) {
        case id_dynamic_keymap_get_keycode:
        case id_dynamic_keymap_set_keycode:
        case id_dynamic_keymap_get_encoder:
        case id_dynamic_keymap_set_encoder:
            return data[1] < POC_VIA_LAYER_COUNT;
        case id_dynamic_keymap_get_buffer:
        case id_dynamic_keymap_set_buffer: {
            uint16_t offset = ((uint16_t)data[1] << 8) | data[2];
            uint16_t visible_bytes = POC_VIA_LAYER_COUNT * MATRIX_ROWS * MATRIX_COLS * 2;
            return data[3] <= length - 4 && offset <= visible_bytes && data[3] <= visible_bytes - offset;
        }
        case id_dynamic_keymap_macro_get_buffer:
        case id_dynamic_keymap_macro_set_buffer:
            return data[3] <= length - 4;
        default:
            return true;
    }
}

// QMK calls via_command_kb() for every raw HID report before VIA parses it.
// In VIA mode the report is VIA traffic; in Codex mode the same endpoint is
// the Codex Micro Channel 2 JSON-RPC path and VIA stays completely silent.
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (!poc_mode_is_codex()) {
        if (!valid_via_command(data, length)) {
            if (length == 32) {
                data[0] = id_unhandled;
                raw_hid_send(data, length);
            }
            return true;
        }
        poc_mode_note_via_command();
        poc_power_note_host_activity();
        return false;
    }

    codex_micro_handle_report(data, length);
    return true;
}
