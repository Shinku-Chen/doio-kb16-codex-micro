#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "test_qmk.h"
#include "codex_micro_protocol.h"
#include "oled_icons.h"
#include "poc_config.h"
#include "poc_power.h"
#include "poc_mode.h"
#include "poc_usb.h"
#include "poc_via.h"
#include "via.h"

layer_state_t layer_state;
layer_state_t default_layer_state = 1UL;
static uint32_t fake_timer;
static uint32_t input_idle;
static bool mode_codex;
static unsigned via_notes;
static unsigned dispatched;
static uint8_t dispatched_lengths[32];
static char outgoing[8192];
static size_t outgoing_length;
static unsigned replies;
static bool rgb_enabled;
static uint8_t rgb_mode;
static uint32_t indicator_colors[RGB_MATRIX_LED_COUNT];
static char screen[4][22];
static uint8_t cursor_row;
static uint8_t cursor_col;
static unsigned oled_clears;
static bool use_saved_keymap;
static uint16_t saved_keymap[4][4][5];
static uint16_t saved_encoders[4][3][2];
static uint8_t user_data[8];
static unsigned keymap_writes;
static unsigned encoder_writes;
static unsigned user_data_writes;
static unsigned replayed_events;
static uint16_t replayed_keycodes[8];
static bool replayed_pressed[8];
static bool replay_changes_layer;
static unsigned replayed_macros;
static uint8_t replayed_macro_id;

uint8_t get_highest_layer(layer_state_t state) {
    for (int bit = 31; bit >= 0; --bit) {
        if (state & (1UL << bit)) {
            return (uint8_t)bit;
        }
    }
    return 0;
}

uint32_t timer_read32(void) { return fake_timer; }
uint16_t timer_read(void) { return (uint16_t)fake_timer; }
uint16_t timer_elapsed(uint16_t start) { return (uint16_t)(fake_timer - start); }
bool poc_mode_is_codex(void) { return mode_codex; }
bool poc_mode_via_waiting(void) { return false; }
void poc_mode_note_via_command(void) { ++via_notes; }
void poc_mode_task(void) {}
uint8_t rgb_matrix_is_enabled(void) { return rgb_enabled; }
uint8_t rgb_matrix_get_mode(void) { return rgb_mode; }
void rgb_matrix_mode_noeeprom(uint8_t mode) { rgb_mode = mode; }
void rgb_matrix_enable_noeeprom(void) { rgb_enabled = true; }
void rgb_matrix_disable_noeeprom(void) { rgb_enabled = false; }
void test_set_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    assert(index < RGB_MATRIX_LED_COUNT);
    indicator_colors[index] = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
}

void oled_clear(void) { memset(screen, 0, sizeof(screen)); ++oled_clears; }
bool oled_on(void) { return true; }
bool oled_off(void) { return false; }
bool rgb_matrix_get_suspend_state(void) { return false; }
uint32_t last_input_activity_elapsed(void) { return input_idle; }
void oled_set_cursor(uint8_t col, uint8_t row) { cursor_col = col; cursor_row = row; }
void oled_write(const char *text, bool invert) {
    (void)invert;
    assert(cursor_row < 4 && cursor_col < 21);
    assert(strlen(text) <= (size_t)(21 - cursor_col));
    strcpy(&screen[cursor_row][cursor_col], text);
}
void oled_write_ln(const char *text, bool invert) {
    oled_write(text, invert);
    cursor_col = 0;
    ++cursor_row;
}
void oled_write_raw_P(const char *text, uint16_t length) { (void)text; assert(length == 512); }
void oled_write_raw(const char *text, uint16_t length) { (void)text; assert(length == 512); }

uint16_t keycode_at_keymap_location_raw(uint8_t layer, uint8_t row, uint8_t col) {
    assert(layer == POC_CODEX_LAYER && row < 4 && col < 5);
    return 0x4100 + row * 5 + col;
}
uint16_t keycode_at_encodermap_location_raw(uint8_t layer, uint8_t index, bool clockwise) {
    assert(layer == POC_CODEX_LAYER && index < 3);
    return 0x4200 + index * 2 + clockwise;
}
uint16_t dynamic_keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col) {
    assert(layer < 4 && row < 4 && col < 5);
    if (use_saved_keymap) {
        return saved_keymap[layer][row][col];
    }
    return 0x1100 + layer * 20 + row * 5 + col;
}
void dynamic_keymap_set_keycode(uint8_t layer, uint8_t row, uint8_t col, uint16_t keycode) {
    assert(use_saved_keymap && layer < 4 && row < 4 && col < 5);
    saved_keymap[layer][row][col] = keycode;
    ++keymap_writes;
}
uint32_t eeconfig_read_user_datablock(void *data, uint32_t offset, uint32_t length) {
    assert(offset + length <= sizeof(user_data));
    memcpy(data, user_data + offset, length);
    return length;
}
uint32_t eeconfig_update_user_datablock(const void *data, uint32_t offset, uint32_t length) {
    assert(offset + length <= sizeof(user_data));
    memcpy(user_data + offset, data, length);
    ++user_data_writes;
    return length;
}
uint16_t dynamic_keymap_get_encoder(uint8_t layer, uint8_t index, bool clockwise) {
    assert(layer < 4 && index < 3);
    if (use_saved_keymap) {
        return saved_encoders[layer][index][clockwise ? 1 : 0];
    }
    return 0x1200 + layer * 6 + index * 2 + clockwise;
}
void dynamic_keymap_set_encoder(uint8_t layer, uint8_t index, bool clockwise, uint16_t keycode) {
    assert(use_saved_keymap && layer < 4 && index < 3);
    saved_encoders[layer][index][clockwise ? 1 : 0] = keycode;
    ++encoder_writes;
}

#include "keymap_under_test.h"

void layer_move(uint8_t layer) { layer_state = layer_state_set_user(1UL << layer); }
void default_layer_set(layer_state_t state) { default_layer_state = default_layer_state_set_user(state); }

void dynamic_keymap_macro_send(uint8_t id) { ++replayed_macros; replayed_macro_id = id; }

#include "qmk_via_key_under_test.h"

void process_record(keyrecord_t *record) {
    uint16_t keycode = keymap_key_to_keycode(get_highest_layer(layer_state | default_layer_state), record->event.key);
    assert(replayed_events < 8 && record->tap.count == 1);
    replayed_keycodes[replayed_events] = keycode;
    replayed_pressed[replayed_events++] = record->event.pressed;
    assert(process_record_user(keycode, record));
    if (!process_record_via(keycode, record)) {
        return;
    }
    if (replay_changes_layer && record->event.pressed) {
        layer_move(0);
    }
}

void raw_hid_send(uint8_t *data, uint8_t length) {
    ++replies;
    if (length == 32) {
        assert(data[0] == id_unhandled);
        return;
    }
    assert(length == 64 && data[0] == 6 && data[1] == 2 && data[2] <= 61);
    assert(outgoing_length + data[2] < sizeof(outgoing));
    memcpy(outgoing + outgoing_length, data + 3, data[2]);
    outgoing_length += data[2];
    outgoing[outgoing_length] = '\0';
}

static void clear_output(void) { outgoing_length = 0; outgoing[0] = '\0'; replies = 0; }

static void feed(const char *json, bool report_id) {
    size_t size = strlen(json);
    for (size_t offset = 0; offset < size;) {
        uint8_t report[64] = {6, 2, 0};
        size_t chunk = size - offset > 61 ? 61 : size - offset;
        report[2] = (uint8_t)chunk;
        memcpy(report + 3, json + offset, chunk);
        via_command_kb(report + !report_id, 64 - !report_id);
        offset += chunk;
    }
}

static unsigned occurrences(const char *needle) {
    unsigned count = 0;
    const char *cursor = outgoing;
    while ((cursor = strstr(cursor, needle)) != NULL) {
        ++count;
        cursor += strlen(needle);
    }
    return count;
}

static void test_ag_independence(void) {
    codex_micro_init();
    clear_output();
    codex_micro_send_agent_key(0, false);
    assert(replies == 0);
    codex_micro_send_agent_key(0, true);
    codex_micro_send_agent_key(0, false);
    assert(occurrences("\"k\":\"AG00\"") == 2);
    assert(occurrences("\"act\":1") == 1 && occurrences("\"act\":0") == 1);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}", true);
    clear_output();
    codex_micro_send_agent_key(0, true);
    codex_micro_send_agent_key(0, false);
    assert(occurrences("\"k\":\"AG00\"") == 2);
    unsigned before = replies;
    codex_micro_send_agent_key(6, true);
    codex_micro_send_agent_key(0, false);
    assert(replies == before);
    puts("PASS: AG input works before host lighting, on empty slots, with paired releases");
}

static void test_protocol_body_and_status(void) {
    codex_micro_init();
    for (unsigned layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
        clear_output();
        layer_state = 1UL << layer;
        feed("{\"method\":\"device.status\",\"id\":810001}", layer % 2 == 0);
        char expected[32];
        snprintf(expected, sizeof(expected), "\"layer_index\":%u", layer == POC_CODEX_LAYER ? 1 : layer + 2);
        assert(strstr(outgoing, expected) && strstr(outgoing, CODEX_MICRO_VERSION));
        assert(occurrences("\"id\":810001") == 1);
    }
    clear_output();
    feed("{\"method\":\"sys.version\",\"id\":810002}", false);
    assert(strstr(outgoing, "\"id\":810002") && strstr(outgoing, CODEX_MICRO_VERSION));
    clear_output();
    uint8_t invalid[64] = {6, 2, 62};
    codex_micro_handle_report(invalid, 64);
    invalid[2] = 20;
    codex_micro_handle_report(invalid, 8);
    codex_micro_handle_report(invalid, 0);
    invalid[1] = 3;
    codex_micro_handle_report(invalid, 64);
    assert(replies == 0);
    const char short_json[] = "{\"method\":\"sys.version\",\"id\":810004}";
    uint8_t short_frame[64] = {2, sizeof(short_json) - 1};
    memcpy(short_frame + 2, short_json, sizeof(short_json) - 1);
    codex_micro_handle_report(short_frame, sizeof(short_json) + 1);
    assert(strstr(outgoing, CODEX_MICRO_VERSION));
    puts("PASS: report-ID normalization, malformed lengths, exact RPC IDs and live layer reporting");
}

static void test_encoders_and_layer_actions(void) {
    feed("{\"method\":\"sys.version\",\"id\":810003}", true);
    clear_output();
    keyrecord_t record = {.event.pressed = true};
    process_record_user(ENC_CW, &record);
    record.event.pressed = false;
    process_record_user(ENC_CW, &record);
    assert(occurrences("\"k\":\"ENC_CW\"") == 1);
    assert(occurrences("\"act\":2") == 1);
    layer_move(POC_CODEX_LAYER);
    record.event.pressed = true;
    fake_timer = 1000;
    process_record_user(LAYER_CYCLE, &record);
    fake_timer = 1000 + POC_LAYER_KEY_HOLD_MS - 1;
    matrix_scan_user();
    assert(get_highest_layer(layer_state) == POC_CODEX_LAYER);
    fake_timer = 1000 + POC_LAYER_KEY_HOLD_MS;
    matrix_scan_user();
    assert(get_highest_layer(layer_state) == 0);
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    assert(process_record_user(TO(POC_CODEX_LAYER), &record));
    layer_move(POC_CODEX_LAYER);
    assert(get_highest_layer(layer_state) == POC_CODEX_LAYER);
    mode_codex = false;
    record.event.pressed = true;
    process_record_user(LAYER_CYCLE, &record);
    fake_timer += POC_LAYER_KEY_HOLD_MS;
    matrix_scan_user();
    assert(get_highest_layer(layer_state) == 0);
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    mode_codex = true;
    for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
        assert(keymap_key_to_keycode(layer, (keypos_t){0, 0}) == (layer == POC_CODEX_LAYER ? LAYER_CYCLE : 0x1100 + layer * 20));
        assert(keymap_key_to_keycode(layer, (keypos_t){KEYLOC_ENCODER_CW, 2}) == (layer == POC_CODEX_LAYER ? 0x4205 : 0x1205 + layer * 6));
    }
    assert(keymap_key_to_keycode(4, (keypos_t){0, 0}) == KC_NO);
    layer_move(4);
    assert(get_highest_layer(layer_state | default_layer_state) == 0);
    puts("PASS: encoder once per detent, one-second L3 exit in either identity and four-layer routing");
}

static void test_ambient_layer_key_priority(void) {
    codex_micro_init();
    default_layer_set(1UL);
    layer_move(POC_CODEX_LAYER);
    layer_key_pressed = false;
    fake_timer = 1000;
    feed("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":3166206,\"b\":1,\"e\":1}}}", true);
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    for (uint8_t led = 12; led < 16; ++led) {
        assert(indicator_colors[led] == 0x304FFE);
    }
    keyrecord_t record = {.event.pressed = true};
    process_record_user(LAYER_CYCLE, &record);
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    assert(indicator_colors[POC_LAYER_KEY_LED] == 0xFFFFFF);
    for (uint8_t led = 12; led < 16; ++led) {
        assert(indicator_colors[led] == 0x304FFE);
    }
    indicator_colors[POC_LAYER_KEY_LED] = 0x123456;
    rgb_matrix_indicators_advanced_user(1, 16);
    assert(indicator_colors[POC_LAYER_KEY_LED] == 0x123456);
    fake_timer = 1000 + POC_LAYER_KEY_HOLD_MS - 1;
    matrix_scan_user();
    assert(get_highest_layer(layer_state) == POC_CODEX_LAYER);
    fake_timer = 1000 + POC_LAYER_KEY_HOLD_MS;
    matrix_scan_user();
    assert(get_highest_layer(layer_state) == 0);
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    layer_move(POC_CODEX_LAYER);
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    for (uint8_t led = 12; led < 16; ++led) {
        assert(indicator_colors[led] == 0x304FFE);
    }
    puts("PASS: fourth-row ambient preserves layer-key white hold feedback and one-second L3 exit");
}

static void reset_saved_layout(void) {
    use_saved_keymap = true;
    memset(user_data, 0, sizeof(user_data));
    memset(user_data, 0xA5, sizeof(uint32_t));
    keymap_writes = 0;
    encoder_writes = 0;
    user_data_writes = 0;
    for (uint8_t layer = 0; layer < 4; ++layer) {
        for (uint8_t row = 0; row < 4; ++row) {
            for (uint8_t col = 0; col < 5; ++col) {
                saved_keymap[layer][row][col] = 0x1000 + layer * 20 + row * 5 + col;
            }
        }
        for (uint8_t encoder = 0; encoder < 3; ++encoder) {
            for (uint8_t direction = 0; direction < 2; ++direction) {
                saved_encoders[layer][encoder][direction] = 0x1200 + layer * 6 + encoder * 2 + direction;
            }
        }
        saved_keymap[layer][1][4] = TO((layer + 1) % 4);
    }
}

static void test_four_layer_migration(void) {
    const uint16_t legacy[] = {TO(4), MO(4), DF(4), PDF(4), TG(4), OSL(4), TT(4), LT(4, KC_A), LM(4, 2)};
    const uint16_t migrated[] = {TO(3), MO(3), DF(3), PDF(3), TG(3), OSL(3), TT(3), LT(3, KC_A), LM(3, 2)};
    reset_saved_layout();
    uint16_t expected_keys[4][4][5];
    uint16_t expected_encoders[4][3][2];
    for (size_t index = 0; index < sizeof(legacy) / sizeof(legacy[0]); ++index) {
        saved_keymap[0][index / 5][index % 5] = legacy[index];
    }
    saved_keymap[3][0][0] = TO(4);
    saved_encoders[0][1][1] = TO(4);
    saved_encoders[2][2][0] = LT(4, KC_B);
    saved_encoders[3][0][0] = MO(4);
    memcpy(expected_keys, saved_keymap, sizeof(expected_keys));
    memcpy(expected_encoders, saved_encoders, sizeof(expected_encoders));
    for (size_t index = 0; index < sizeof(migrated) / sizeof(migrated[0]); ++index) {
        expected_keys[0][index / 5][index % 5] = migrated[index];
    }
    expected_encoders[0][1][1] = TO(3);
    expected_encoders[2][2][0] = LT(3, KC_B);
    poc_migrate_four_layer_layout();
    assert(memcmp(expected_keys, saved_keymap, sizeof(expected_keys)) == 0);
    assert(memcmp(expected_encoders, saved_encoders, sizeof(expected_encoders)) == 0);
    assert(keymap_writes == 9 && encoder_writes == 2 && user_data_writes == 1);
    assert(poc_remap_legacy_codex_keycode(KC_A) == KC_A);
    assert(poc_remap_legacy_codex_keycode(LT(2, KC_A)) == LT(2, KC_A));
    assert(poc_remap_legacy_codex_keycode(LM(1, 2)) == LM(1, 2));
    assert(poc_remap_legacy_codex_keycode(TO(0)) == TO(0));
    for (size_t offset = 0; offset < sizeof(uint32_t); ++offset) {
        assert(user_data[offset] == 0xA5);
    }
    uint32_t stamp;
    memcpy(&stamp, user_data + sizeof(uint32_t), sizeof(stamp));
    assert(stamp == POC_FOUR_LAYER_STAMP);
    saved_keymap[0][0][0] = TO(4);
    saved_encoders[0][1][1] = TO(4);
    poc_migrate_four_layer_layout();
    assert(saved_keymap[0][0][0] == TO(4) && saved_encoders[0][1][1] == TO(4));
    assert(keymap_writes == 9 && encoder_writes == 2 && user_data_writes == 1);
    use_saved_keymap = false;
    puts("PASS: legacy layer-4 actions migrate to layer 3 once; page 3 and unrelated keys/encoders remain intact");
}

static void test_placeholder_page(void) {
    reset_saved_layout();
    mode_codex = false;
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            uint8_t report[32] = {id_dynamic_keymap_set_keycode, 3, row, col, 0, KC_A};
            assert(!via_command_kb(report, sizeof(report)));
            dynamic_keymap_set_keycode(3, row, col, KC_A);
            assert(dynamic_keymap_get_keycode(3, row, col) == KC_A);
            uint16_t expected = poc_codex_custom_position((keypos_t){row, col}) ? (row == POC_LAYER_KEY_ROW && col == POC_LAYER_KEY_COL ? LAYER_CYCLE : KC_A) : 0x4100 + row * 5 + col;
            assert(keymap_key_to_keycode(3, (keypos_t){row, col}) == expected);
        }
    }
    for (uint8_t encoder = 0; encoder < NUM_ENCODERS; ++encoder) {
        for (uint8_t direction = 0; direction < 2; ++direction) {
            uint8_t report[32] = {id_dynamic_keymap_set_encoder, 3, encoder, direction, 0, KC_B};
            assert(!via_command_kb(report, sizeof(report)));
            dynamic_keymap_set_encoder(3, encoder, direction != 0, KC_B);
            assert(dynamic_keymap_get_encoder(3, encoder, direction != 0) == KC_B);
            keypos_t location = {direction != 0 ? KEYLOC_ENCODER_CW : KEYLOC_ENCODER_CCW, encoder};
            assert(keymap_key_to_keycode(3, location) == 0x4200 + encoder * 2 + direction);
        }
    }
    dynamic_keymap_set_keycode(0, 0, 0, KC_C);
    assert(keymap_key_to_keycode(0, (keypos_t){0, 0}) == KC_C);
    use_saved_keymap = false;
    mode_codex = true;
    puts("PASS: VIA page 3 executes only five custom positions, retains the exit sentinel and ignores edits to native keys/encoder rotation");
}

static void test_custom_migration(void) {
    reset_saved_layout();
    uint16_t before_keys[4][4][5];
    uint16_t before_encoders[4][3][2];
    memcpy(before_keys, saved_keymap, sizeof(before_keys));
    memcpy(before_encoders, saved_encoders, sizeof(before_encoders));
    uint32_t stamp = POC_FOUR_LAYER_STAMP;
    memcpy(user_data + sizeof(uint32_t), &stamp, sizeof(stamp));
    poc_migrate_codex_custom_keys();
    assert(keymap_writes == 5 && encoder_writes == 0 && user_data_writes == 1);
    for (uint8_t layer = 0; layer < 4; ++layer) {
        for (uint8_t row = 0; row < 4; ++row) {
            for (uint8_t column = 0; column < 5; ++column) {
                bool reset = layer == 3 && poc_codex_custom_position((keypos_t){row, column});
                assert(saved_keymap[layer][row][column] == (reset ? KC_NO : before_keys[layer][row][column]));
            }
        }
    }
    assert(memcmp(before_encoders, saved_encoders, sizeof(before_encoders)) == 0);
    saved_keymap[3][0][0] = KC_C;
    saved_keymap[0][0][0] = TO(4);
    poc_migrate_four_layer_layout();
    poc_migrate_codex_custom_keys();
    assert(saved_keymap[3][0][0] == KC_C && saved_keymap[0][0][0] == TO(4));
    assert(keymap_writes == 5 && user_data_writes == 1);
    assert(user_data[0] == 0xA5);
    use_saved_keymap = false;
    puts("PASS: custom-key migration clears only the five newly active positions once, retaining normal pages, native storage, encoders and the existing EEPROM layout");
}

static void test_custom_tap_and_hold(void) {
    reset_saved_layout();
    default_layer_set(1UL);
    layer_move(3);
    saved_keymap[3][POC_LAYER_KEY_ROW][POC_LAYER_KEY_COL] = QK_MACRO_0;
    replayed_events = 0;
    replayed_macros = 0;
    keyrecord_t record = {.event = {.key = {POC_LAYER_KEY_ROW, POC_LAYER_KEY_COL}, .pressed = true}};
    fake_timer = 100;
    process_record_user(LAYER_CYCLE, &record);
    assert(replayed_events == 0);
    saved_keymap[3][POC_LAYER_KEY_ROW][POC_LAYER_KEY_COL] = KC_B;
    fake_timer += 999;
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    assert(replayed_events == 2 && replayed_keycodes[0] == QK_MACRO_0 && replayed_keycodes[1] == QK_MACRO_0);
    assert(replayed_pressed[0] && !replayed_pressed[1] && !layer_key_replaying);
    assert(replayed_macros == 1 && replayed_macro_id == 0);
    record.event.pressed = true;
    process_record_user(LAYER_CYCLE, &record);
    fake_timer += 1000;
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    assert(replayed_events == 2 && get_highest_layer(layer_state) == 0);
    layer_move(3);
    record.event.pressed = true;
    process_record_user(LAYER_CYCLE, &record);
    fake_timer += 1000;
    matrix_scan_user();
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    assert(replayed_events == 2 && get_highest_layer(layer_state) == 0);
    layer_move(3);
    record.event.pressed = true;
    process_record_user(LAYER_CYCLE, &record);
    layer_move(1);
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    assert(replayed_events == 2 && get_highest_layer(layer_state) == 1);
    layer_move(3);
    saved_keymap[3][POC_LAYER_KEY_ROW][POC_LAYER_KEY_COL] = TO(0);
    replay_changes_layer = true;
    record.event.pressed = true;
    process_record_user(LAYER_CYCLE, &record);
    record.event.pressed = false;
    process_record_user(LAYER_CYCLE, &record);
    assert(replayed_events == 4 && replayed_keycodes[2] == TO(0) && replayed_keycodes[3] == TO(0));
    assert(get_highest_layer(layer_state) == 0 && !layer_key_replaying);
    replay_changes_layer = false;
    replayed_events = 0;
    use_saved_keymap = false;
    puts("PASS: exit short taps replay paired QMK events after release, snapshot the mapping and survive a layer action; long holds never emit the tap, even without an intervening scan");
}

static void test_custom_key_safety(void) {
    reset_saved_layout();
    const keypos_t positions[] = {{3,0}, {0,3}, {0,4}, {1,4}};
    for (size_t index = 0; index < sizeof(positions) / sizeof(positions[0]); ++index) {
        keypos_t key = positions[index];
        saved_keymap[3][key.row][key.col] = KC_TRNS;
        assert(keymap_key_to_keycode(3, key) == KC_NO);
        saved_keymap[3][key.row][key.col] = AG00;
        assert(keymap_key_to_keycode(3, key) == KC_NO);
        saved_keymap[3][key.row][key.col] = KC_LCTL;
        assert(keymap_key_to_keycode(3, key) == KC_LCTL);
    }
    assert(keymap_key_to_keycode(3, (keypos_t){2,4}) == 0x410E);
    assert(keymap_key_to_keycode(3, (keypos_t){POC_LAYER_KEY_ROW,POC_LAYER_KEY_COL}) == LAYER_CYCLE);
    use_saved_keymap = false;
    puts("PASS: the four direct custom keys accept held modifiers without inheriting L0 or injecting native AG/ACT events; encoder C and the long-hold exit remain fixed");
}

static void test_bottom_left_is_direct(void) {
    reset_saved_layout();
    default_layer_set(1UL);
    layer_move(POC_CODEX_LAYER);
    layer_key_pressed = false;
    saved_keymap[3][3][0] = KC_LCTL;
    keyrecord_t record = {.event = {.key = {3, 0}, .pressed = true}};
    uint16_t keycode = keymap_key_to_keycode(3, record.event.key);
    assert(keycode == KC_LCTL && process_record_user(keycode, &record));
    fake_timer += POC_LAYER_KEY_HOLD_MS * 2;
    matrix_scan_user();
    assert(!layer_key_pressed && get_highest_layer(layer_state | default_layer_state) == 3);
    record.event.pressed = false;
    assert(process_record_user(keycode, &record));
    assert(keymap_key_to_keycode(3, (keypos_t){0,0}) == LAYER_CYCLE);
    use_saved_keymap = false;
    puts("PASS: bottom-left is a direct VIA key with held modifiers and no forced exit after two seconds; the exit is exclusively top-left");
}

static void test_rgb_restore(void) {
    layer_state = 0;
    default_layer_state = 1UL;
    codex_rgb_ready = false;
    codex_rgb_active = false;
    rgb_enabled = false;
    rgb_mode = 7;
    default_layer_set(1UL << 3);
    assert(!rgb_enabled && rgb_mode == 7);
    codex_rgb_init();
    layer_move(3);
    assert(rgb_enabled && rgb_mode == RGB_MATRIX_SOLID_COLOR);
    default_layer_set(1UL);
    layer_move(0);
    assert(!rgb_enabled && rgb_mode == 7);
    layer_move(3);
    layer_move(3);
    layer_move(2);
    assert(!rgb_enabled && rgb_mode == 7);
    rgb_enabled = true;
    rgb_mode = 9;
    layer_move(3);
    layer_move(1);
    assert(rgb_enabled && rgb_mode == 9);
    default_layer_set(1UL << 3);
    assert(rgb_enabled && rgb_mode == RGB_MATRIX_SOLID_COLOR);
    assert(get_highest_layer(layer_state | default_layer_state) == 3);
    exit_codex_layer();
    assert(default_layer_state == 1UL && get_highest_layer(layer_state) == 0);
    assert(rgb_enabled && rgb_mode == 9);
    default_layer_set(1UL << 4);
    layer_move(4);
    assert(default_layer_state == 1UL && layer_state == 0);
    puts("PASS: L3 RGB handles default layers, delays hardware setup and restores ordinary settings; layer 4 is rejected");
}

static void test_via_bounds(void) {
    mode_codex = false;
    via_notes = 0;
    clear_output();
    uint8_t report[32] = {id_dynamic_keymap_set_keycode, 0, 0, 0, 0x52, 0x03};
    assert(!via_command_kb(report, 32));
    assert(via_notes == 1 && report[4] == 0x52 && report[5] == 0x03);
    report[1] = 4;
    assert(via_command_kb(report, 32) && report[0] == id_unhandled);
    assert(via_notes == 1 && replies == 1);
    uint8_t buffer[32] = {id_dynamic_keymap_set_buffer, 0, 158, 2};
    assert(!via_command_kb(buffer, 32));
    buffer[3] = 3;
    assert(via_command_kb(buffer, 32) && buffer[0] == id_unhandled);
    buffer[0] = id_dynamic_keymap_get_buffer;
    buffer[2] = 0;
    buffer[3] = 29;
    assert(via_command_kb(buffer, 32) && buffer[0] == id_unhandled);
    buffer[0] = 0;
    unsigned before = via_notes;
    assert(via_command_kb(buffer, 32) && via_notes == before);
    mode_codex = true;
    puts("PASS: VIA exposes four stored pages, rejects out-of-range addresses and accepts TO(3)");
}

typedef struct { uint8_t *ptr, *top; } input_buffers_queue_t;
typedef struct { input_buffers_queue_t ibqueue; } endpoint_t;
static endpoint_t usb_endpoints_out[1];
static uint8_t input_packets[16][64];
static uint8_t input_lengths[16];
static unsigned input_count;
static unsigned input_front;
static bool usb_active = true;
static int usb_driver;
#define RAW_EPSIZE 64
#define RAW_REPORT_ID 6
#define RAW_INTERFACE 3
#define USB_ENDPOINT_OUT_RAW 0
#define USB_DRIVER usb_driver
#define USB_ACTIVE 1
#define TIME_IMMEDIATE 0
#define MSG_OK 0
static void osalSysLock(void) {}
static void osalSysUnlock(void) {}
static int usbGetDriverStateI(const void *driver) { (void)driver; return usb_active; }
static int ibqGetFullBufferTimeoutS(input_buffers_queue_t *queue, int timeout) {
    (void)timeout;
    if (input_front == input_count) { return -1; }
    queue->ptr = input_packets[input_front];
    queue->top = queue->ptr + input_lengths[input_front];
    return MSG_OK;
}
static void ibqReleaseEmptyBufferS(input_buffers_queue_t *queue) { (void)queue; ++input_front; }
void raw_hid_receive(uint8_t *data, uint8_t length) {
    (void)data;
    assert(dispatched < sizeof(dispatched_lengths));
    dispatched_lengths[dispatched++] = length;
}
#include "usb_task_under_test.h"

typedef struct {
    struct { uint8_t lbyte, hbyte; } wValue;
    uint16_t wLength;
} test_setup_t;
static uint8_t raw_set_report_length;
static uint8_t raw_set_report_buf[64];
static void raw_set_report_complete(void *driver) { (void)driver; assert(poc_usb_control_push(raw_set_report_buf, raw_set_report_length)); }
static unsigned setup_transfers;
static void usbSetupTransfer(void *driver, uint8_t *buffer, uint16_t length, void (*callback)(void *)) {
    (void)driver; (void)buffer; (void)length; (void)callback;
    ++setup_transfers;
}
static bool setup_report(test_setup_t *setup) {
    void *usbp = NULL;
    switch (RAW_INTERFACE) {
#include "usb_setup_under_test.h"
    }
    return false;
}

static void test_control_queue_and_packets(void) {
    poc_usb_select(POC_USB_DESC_CODEX);
    uint8_t source[64] = {6, 2, 0};
    uint8_t dest[64], length;
    assert(!poc_usb_control_push(source, 0) && !poc_usb_control_push(source, 65));
    for (uint8_t index = 0; index < 15; ++index) {
        source[3] = index;
        assert(poc_usb_control_push(source, index % 2 ? 63 : 64));
    }
    assert(!poc_usb_control_ready() && !poc_usb_control_push(source, 64));
    test_setup_t setup = {.wValue = {6, 2}, .wLength = 64};
    assert(!setup_report(&setup) && setup_transfers == 0);
    for (uint8_t index = 0; index < 15; ++index) {
        assert(poc_usb_control_pop(dest, &length));
        assert(dest[3] == index && length == (index % 2 ? 63 : 64));
    }
    assert(!poc_usb_control_pop(dest, &length));
    for (unsigned index = 0; index < 40; ++index) {
        assert(poc_usb_control_push(source, 64));
        assert(poc_usb_control_pop(dest, &length));
    }
    assert(setup_report(&setup) && raw_set_report_length == 64);
    setup.wLength = 63;
    assert(setup_report(&setup) && raw_set_report_length == 63);
    setup.wValue.hbyte = 1;
    assert(!setup_report(&setup));
    setup.wValue.hbyte = 2;
    setup.wLength = 65;
    assert(!setup_report(&setup));
    setup.wLength = 64;
    setup.wValue.lbyte = 5;
    assert(!setup_report(&setup));
    assert(poc_usb_control_push(source, 64));
    poc_usb_select(POC_USB_DESC_VIA);
    assert(poc_usb_via_mode() && poc_raw_hid_epsize() == 32 && !poc_usb_control_pop(dest, &length));
    setup.wValue.lbyte = 0;
    setup.wLength = 32;
    assert(setup_report(&setup));
    raw_set_report_complete(NULL);
    input_count = 4;
    input_front = 0;
    input_lengths[0] = 32;
    input_lengths[1] = 32;
    input_lengths[2] = 63;
    input_lengths[3] = 64;
    dispatched = 0;
    raw_hid_task();
    const uint8_t expected[] = {32, 32, 32, 63, 64};
    assert(dispatched == sizeof(expected) && memcmp(expected, dispatched_lengths, sizeof(expected)) == 0);
    assert(input_front == input_count);
    usb_active = false;
    raw_hid_task();
    assert(dispatched == sizeof(expected));
    puts("PASS: EP0 validation/deferred queue, full/wrap/reset handling and separate 32/63/64-byte packets");
}

typedef struct __attribute__((packed)) { uint8_t header[7]; uint16_t HIDReportLength; } USB_HID_Descriptor_HID_t;
static const struct { USB_HID_Descriptor_HID_t Raw_HID; } ConfigurationDescriptor = {.Raw_HID.HIDReportLength = 36};
static struct { USB_HID_Descriptor_HID_t Raw_HID; } poc_via_configuration;
static void poc_prepare_via_descriptors(void) { poc_via_configuration.Raw_HID.HIDReportLength = 34; }

static void test_hid_descriptor_selection(void) {
    for (unsigned mode = 0; mode < 2; ++mode) {
        poc_usb_select(mode == 0 ? POC_USB_DESC_VIA : POC_USB_DESC_CODEX);
        const void *Address = NULL;
        uint16_t Size = 0;
        switch (RAW_INTERFACE) {
#include "hid_descriptor_under_test.h"
        }
        assert(Address != NULL && Size == 9);
        const USB_HID_Descriptor_HID_t *descriptor = Address;
        assert(descriptor->HIDReportLength == (mode == 0 ? 34 : 36));
    }
    puts("PASS: standalone HID descriptor selects the matching VIA/Codex configuration header");
}


static void test_shared_rgb_power(void) {
    fake_timer = 0;
    input_idle = POC_IDLE_SLEEP_MS - 1;
    layer_state = default_layer_state = 1UL;
    layer_key_pressed = false;
    codex_micro_init();
    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; ++led) indicator_colors[led] = 0x112233;
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; ++led) assert(indicator_colors[led] == 0x112233);
    fake_timer = input_idle = POC_IDLE_SLEEP_MS;
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; ++led) assert(indicator_colors[led] == 0);
    mode_codex = false;
    uint8_t via[32] = {id_get_protocol_version};
    assert(!via_command_kb(via, sizeof(via)));
    assert(!poc_power_should_sleep());
    mode_codex = true;
    layer_state = 1UL << POC_CODEX_LAYER;
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":3166206,\"b\":1,\"e\":1}]}", true);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_06, true);
    layer_key_pressed = true;
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    assert(indicator_colors[1] == 0x304FFE && indicator_colors[8] == 0xFFFFFF && indicator_colors[POC_LAYER_KEY_LED] == 0xFFFFFF && indicator_colors[12] == 0);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}", true);
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; ++led) assert(indicator_colors[led] == 0);
    feed("{\"method\":\"device.status\",\"id\":810009}", true);
    assert(poc_power_should_sleep());
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":1}]}", true);
    rgb_matrix_indicators_advanced_user(0, RGB_MATRIX_LED_COUNT);
    assert(indicator_colors[1] == 0x304FFE && indicator_colors[8] == 0xFFFFFF && indicator_colors[POC_LAYER_KEY_LED] == 0xFFFFFF && indicator_colors[12] == 0);
    layer_key_pressed = false;
    input_idle = 0;
    puts("PASS: real RGB indicator gates VIA/Codex output with shared power, valid VIA packets renew it, and explicit all-off outranks local ACT/layer white without losing host colors");
}

int main(void) {
    mode_codex = true;
    test_rgb_restore();
    test_ag_independence();
    test_protocol_body_and_status();
    test_encoders_and_layer_actions();
    test_ambient_layer_key_priority();
    test_four_layer_migration();
    test_placeholder_page();
    test_custom_migration();
    test_custom_tap_and_hold();
    test_custom_key_safety();
    test_bottom_left_is_direct();
    test_via_bounds();
    test_control_queue_and_packets();
    test_hid_descriptor_selection();
    test_shared_rgb_power();
    clear_output();
    return 0;
}
