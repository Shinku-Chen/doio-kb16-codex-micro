#include QMK_KEYBOARD_H

#include "codex_micro_protocol.h"
#include "oled_icons.h"
#include "poc_config.h"
#include "poc_power.h"
#include "poc_mode.h"

#include "keymap_introspection.h"

enum layer_names {
    _VIA0 = 0,
    _VIA1,
    _VIA2,
    _CODEX = POC_CODEX_LAYER,
};

enum poc_keycodes {
    AG00 = QK_KB_0,
    AG01,
    AG02,
    AG03,
    AG04,
    AG05,
    ACT06,
    ACT07,
    ACT08,
    ACT09,
    ACT10,
    ACT11,
    ACT12,
    JOY_UP,
    JOY_DOWN,
    JOY_LEFT,
    JOY_RIGHT,
    ENC_CC,
    ENC_CW,
    ENC_SW,
    LAYER_CYCLE,
};

#define POC_LAYER_STAMP 0x4B423136UL // "KB16"
#define POC_FOUR_LAYER_STAMP 0x344C4333UL
#define POC_CUSTOM_KEYS_STAMP 0x354B4333UL

/* Layers 0-2 mirror the VIA layout that was saved on the stock firmware.
 * Layer 3 keeps native AG/ACT, joystick axes and encoder controls in flash.
 * Five spare key positions use VIA; top-left retains a one-second exit. */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_VIA0] = LAYOUT(
        KC_ESC, G(KC_W), LSG(KC_A), G(KC_BSPC), TO(0),
        G(KC_C), G(KC_V), LSG(KC_F), KC_ENT, TO(1),
        C(KC_D), G(KC_RBRC), C(KC_R), G(KC_F2), KC_MPLY,
        MT(MOD_LGUI, KC_MUTE), G(KC_LBRC), LAG(KC_R), G(KC_Z)),

    [_VIA1] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_PASTE, QK_MACRO_0, QK_MACRO_1, KC_ENT, TO(2),
        KC_F10, KC_F11, S(KC_F11), KC_F9, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_F8, KC_TRNS),

    [_VIA2] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, TO(3),
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, MS_BTN1, MS_BTN1, MS_BTN1),

    [_CODEX] = LAYOUT(
        KC_NO, AG00, AG01, KC_NO, KC_NO,  /* [0,4] 旋钮 A 按下：VIA 默认无动作 */
        AG02, AG03, AG04, AG05, KC_NO,    /* [1,4] 旋钮 B 按下：VIA 默认无动作 */
        ACT06, ACT07, ACT08, ACT09, ENC_SW, /* [2,4] 旋钮 C 按下 = 编码器按键 */
        KC_NO, ACT10, ACT11, ACT12),
};

#ifdef ENCODER_MAP_ENABLE
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_VIA0] = {
        ENCODER_CCW_CW(KC_BRID, KC_BRIU),
        ENCODER_CCW_CW(KC_VOLD, KC_VOLU),
        ENCODER_CCW_CW(KC_MPRV, KC_MNXT),
    },
    [_VIA1] = {
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
    },
    [_VIA2] = {
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
        ENCODER_CCW_CW(MS_BTN1, MS_BTN1),
    },
    /* A/B are the joystick X/Y axes, C is the Codex encoder. */
    [_CODEX] = {
        ENCODER_CCW_CW(JOY_LEFT, JOY_RIGHT),
        ENCODER_CCW_CW(JOY_DOWN, JOY_UP),
        ENCODER_CCW_CW(ENC_CC, ENC_CW),
    },
};
#endif

#define POC_LAYER_COUNT (POC_CODEX_LAYER + 1)

static bool     layer_key_pressed;
static bool     layer_key_fired;
static uint16_t layer_key_at;
static uint16_t layer_key_tap_code;
static bool     layer_key_replaying;

static bool poc_codex_custom_position(keypos_t key) {
    return (key.row == 0 && (key.col == 0 || key.col == 3 || key.col == 4)) ||
           (key.row == 1 && key.col == 4) || (key.row == 3 && key.col == 0);
}

static uint16_t poc_codex_custom_keycode(keypos_t key) {
    uint16_t keycode = dynamic_keymap_get_keycode(POC_CODEX_LAYER, key.row, key.col);
    if (keycode == KC_TRNS || (keycode >= AG00 && keycode <= LAYER_CYCLE)) {
        return KC_NO;
    }
    return keycode;
}

static void exit_codex_layer(void) {
    default_layer_set(1UL);
    layer_move(0);
}

// Layers 0-2 use VIA data; layer 3 uses VIA only at the five custom positions.
// QMK's own keymap_key_to_keycode() is weak, so this replaces it wholesale.
uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key) {
    if (layer >= POC_VIA_LAYER_COUNT) {
        return KC_NO;
    }
    if (layer_key_replaying && key.row == POC_LAYER_KEY_ROW && key.col == POC_LAYER_KEY_COL) {
        return layer_key_tap_code;
    }
    if (layer == POC_CODEX_LAYER) {
        if (key.row < MATRIX_ROWS && key.col < MATRIX_COLS) {
            if (poc_codex_custom_position(key)) {
                return key.row == POC_LAYER_KEY_ROW && key.col == POC_LAYER_KEY_COL ? LAYER_CYCLE : poc_codex_custom_keycode(key);
            }
            return keycode_at_keymap_location_raw(layer, key.row, key.col);
        }
#ifdef ENCODER_MAP_ENABLE
        if (key.row == KEYLOC_ENCODER_CW && key.col < NUM_ENCODERS) {
            return keycode_at_encodermap_location_raw(layer, key.col, true);
        }
        if (key.row == KEYLOC_ENCODER_CCW && key.col < NUM_ENCODERS) {
            return keycode_at_encodermap_location_raw(layer, key.col, false);
        }
#endif
        return KC_NO;
    }

    if (key.row < MATRIX_ROWS && key.col < MATRIX_COLS) {
        return dynamic_keymap_get_keycode(layer, key.row, key.col);
    }
#ifdef ENCODER_MAP_ENABLE
    if (key.row == KEYLOC_ENCODER_CW && key.col < NUM_ENCODERS) {
        return dynamic_keymap_get_encoder(layer, key.col, true);
    }
    if (key.row == KEYLOC_ENCODER_CCW && key.col < NUM_ENCODERS) {
        return dynamic_keymap_get_encoder(layer, key.col, false);
    }
#endif
    return KC_NO;
}

static void codex_send_action(codex_micro_action_t action, bool pressed) {
    codex_micro_send_action_key(action, pressed);
}

static void poc_send_layer_tap(keyrecord_t *record) {
    if (layer_key_tap_code == KC_NO) {
        return;
    }
    keyrecord_t replay = *record;
    replay.event.key = (keypos_t){.row = POC_LAYER_KEY_ROW, .col = POC_LAYER_KEY_COL};
#if defined(COMBO_ENABLE) || defined(REPEAT_KEY_ENABLE)
    replay.keycode = layer_key_tap_code;
#endif
#ifndef NO_ACTION_TAPPING
    replay.tap.count = 1;
    replay.tap.interrupted = false;
#endif
    layer_key_replaying = true;
    replay.event.pressed = true;
    process_record(&replay);
    replay.event.pressed = false;
    process_record(&replay);
    layer_key_replaying = false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LAYER_CYCLE:
            // L3 左上角长按离开 Codex；普通层由 VIA 配置 TO(0)..TO(3)。
            if (record->event.pressed) {
                layer_key_pressed  = true;
                layer_key_fired    = false;
                layer_key_at       = timer_read();
                layer_key_tap_code = poc_codex_custom_keycode((keypos_t){.row = POC_LAYER_KEY_ROW, .col = POC_LAYER_KEY_COL});
            } else {
                bool short_tap = layer_key_pressed && !layer_key_fired && timer_elapsed(layer_key_at) < POC_LAYER_KEY_HOLD_MS && get_highest_layer(layer_state | default_layer_state) == POC_CODEX_LAYER;
                bool long_hold = layer_key_pressed && !layer_key_fired && timer_elapsed(layer_key_at) >= POC_LAYER_KEY_HOLD_MS;
                layer_key_pressed = false;
                if (long_hold) {
                    layer_key_fired = true;
                    exit_codex_layer();
                } else if (short_tap) {
                    poc_send_layer_tap(record);
                }
            }
            return false;
        case AG00:
        case AG01:
        case AG02:
        case AG03:
        case AG04:
        case AG05:
            codex_micro_send_agent_key((uint8_t)(keycode - AG00), record->event.pressed);
            return false;
        case ACT06:
            codex_send_action(CODEX_MICRO_ACTION_06, record->event.pressed);
            return false;
        case ACT07:
            codex_send_action(CODEX_MICRO_ACTION_07, record->event.pressed);
            return false;
        case ACT08:
            codex_send_action(CODEX_MICRO_ACTION_08, record->event.pressed);
            return false;
        case ACT09:
            codex_send_action(CODEX_MICRO_ACTION_09, record->event.pressed);
            return false;
        case ACT10:
            codex_send_action(CODEX_MICRO_ACTION_MIC, record->event.pressed);
            return false;
        case ACT11:
            codex_send_action(CODEX_MICRO_ACTION_11, record->event.pressed);
            return false;
        case ACT12:
            codex_send_action(CODEX_MICRO_ACTION_12, record->event.pressed);
            return false;
        case JOY_UP:
            codex_micro_send_joystick_direction(CODEX_MICRO_JOYSTICK_UP, record->event.pressed);
            return false;
        case JOY_RIGHT:
            codex_micro_send_joystick_direction(CODEX_MICRO_JOYSTICK_RIGHT, record->event.pressed);
            return false;
        case JOY_DOWN:
            codex_micro_send_joystick_direction(CODEX_MICRO_JOYSTICK_DOWN, record->event.pressed);
            return false;
        case JOY_LEFT:
            codex_micro_send_joystick_direction(CODEX_MICRO_JOYSTICK_LEFT, record->event.pressed);
            return false;
        case ENC_CC:
            if (record->event.pressed) {
                codex_micro_send_encoder_turn(false);
            }
            return false;
        case ENC_CW:
            if (record->event.pressed) {
                codex_micro_send_encoder_turn(true);
            }
            return false;
        case ENC_SW:
            codex_micro_send_encoder_press(record->event.pressed);
            return false;
        default:
            break;
    }
    return true;
}

void matrix_scan_user(void) {
    if (layer_key_pressed && !layer_key_fired && timer_elapsed(layer_key_at) >= POC_LAYER_KEY_HOLD_MS) {
        layer_key_fired = true;
        exit_codex_layer();
    }

    poc_mode_task();
    codex_micro_task();
}

void keyboard_pre_init_user(void) {
    poc_mode_init();
}

static void poc_check_layout_stamp(void) {
    uint32_t stamp = 0;
    if (eeconfig_is_user_datablock_valid()) {
        eeconfig_read_user_datablock(&stamp, 0, sizeof(stamp));
    }
    if (stamp == POC_LAYER_STAMP) {
        return;
    }

    // Another firmware may have left a valid looking EEPROM behind. Wipe it so
    // the VIA keymap starts from the defaults in this file.
    eeconfig_init();
    uint32_t fresh = POC_LAYER_STAMP;
    eeconfig_update_user_datablock(&fresh, 0, sizeof(fresh));
}

static uint16_t poc_remap_legacy_codex_keycode(uint16_t keycode) {
    if (IS_QK_LAYER_TAP(keycode) && QK_LAYER_TAP_GET_LAYER(keycode) == 4) {
        return (keycode & 0xF0FFU) | (POC_CODEX_LAYER << 8);
    }
    if (IS_QK_LAYER_MOD(keycode) && QK_LAYER_MOD_GET_LAYER(keycode) == 4) {
        return (keycode & 0xFE1FU) | (POC_CODEX_LAYER << 5);
    }
    bool layer_action = IS_QK_TO(keycode) || IS_QK_MOMENTARY(keycode) || IS_QK_DEF_LAYER(keycode) || IS_QK_PERSISTENT_DEF_LAYER(keycode) || IS_QK_TOGGLE_LAYER(keycode) || IS_QK_ONE_SHOT_LAYER(keycode) || IS_QK_LAYER_TAP_TOGGLE(keycode);
    if (layer_action && (keycode & 0x1FU) == 4) {
        return (keycode & 0xFFE0U) | POC_CODEX_LAYER;
    }
    return keycode;
}

static void poc_migrate_four_layer_layout(void) {
    uint32_t stamp = 0;
    eeconfig_read_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
    if (stamp == POC_FOUR_LAYER_STAMP || stamp == POC_CUSTOM_KEYS_STAMP) {
        return;
    }

    for (uint8_t layer = 0; layer < POC_NORMAL_LAYER_COUNT; ++layer) {
        for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
            for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
                uint16_t keycode = dynamic_keymap_get_keycode(layer, row, col);
                uint16_t migrated = poc_remap_legacy_codex_keycode(keycode);
                if (migrated != keycode) {
                    dynamic_keymap_set_keycode(layer, row, col, migrated);
                }
            }
        }
#ifdef ENCODER_MAP_ENABLE
        for (uint8_t encoder = 0; encoder < NUM_ENCODERS; ++encoder) {
            for (uint8_t direction = 0; direction < NUM_DIRECTIONS; ++direction) {
                bool clockwise = direction != 0;
                uint16_t keycode = dynamic_keymap_get_encoder(layer, encoder, clockwise);
                uint16_t migrated = poc_remap_legacy_codex_keycode(keycode);
                if (migrated != keycode) {
                    dynamic_keymap_set_encoder(layer, encoder, clockwise, migrated);
                }
            }
        }
#endif
    }
    stamp = POC_FOUR_LAYER_STAMP;
    eeconfig_update_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
}

static void poc_migrate_codex_custom_keys(void) {
    uint32_t stamp = 0;
    eeconfig_read_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
    if (stamp == POC_CUSTOM_KEYS_STAMP) {
        return;
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t column = 0; column < MATRIX_COLS; ++column) {
            if (poc_codex_custom_position((keypos_t){.row = row, .col = column})) {
                dynamic_keymap_set_keycode(POC_CODEX_LAYER, row, column, KC_NO);
            }
        }
    }
    stamp = POC_CUSTOM_KEYS_STAMP;
    eeconfig_update_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
}

#ifdef RGB_MATRIX_ENABLE
static void codex_rgb_init(void);
#endif

void keyboard_post_init_user(void) {
    poc_check_layout_stamp();
    poc_migrate_four_layer_layout();
    poc_migrate_codex_custom_keys();
    codex_micro_init();
    oled_controller_init();
#ifdef RGB_MATRIX_ENABLE
    codex_rgb_init();
#endif
    if (poc_mode_is_codex()) {
        layer_move(POC_CODEX_LAYER);
    } else {
        layer_move(0);
        poc_mode_start_window();
    }
}

#ifdef RGB_MATRIX_ENABLE
static bool codex_rgb_active;
static bool codex_rgb_ready;
static bool saved_rgb_enabled;
static uint8_t saved_rgb_mode;

static void codex_rgb_init(void) {
    codex_rgb_ready = true;
}

static void update_codex_rgb(uint8_t layer) {
    if (!codex_rgb_ready) {
        return;
    }
    bool is_codex = layer == POC_CODEX_LAYER;
    if (is_codex && !codex_rgb_active) {
        saved_rgb_enabled = rgb_matrix_is_enabled();
        saved_rgb_mode = rgb_matrix_get_mode();
        rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
        rgb_matrix_enable_noeeprom();
    } else if (!is_codex && codex_rgb_active) {
        rgb_matrix_mode_noeeprom(saved_rgb_mode);
        if (!saved_rgb_enabled) {
            rgb_matrix_disable_noeeprom();
        }
    }
    codex_rgb_active = is_codex;
}

layer_state_t layer_state_set_user(layer_state_t state) {
    state &= (1UL << POC_VIA_LAYER_COUNT) - 1;
    update_codex_rgb(get_highest_layer(state | default_layer_state));
    return state;
}

layer_state_t default_layer_state_set_user(layer_state_t state) {
    state &= (1UL << POC_VIA_LAYER_COUNT) - 1;
    if (state == 0) {
        state = 1UL;
    }
    update_codex_rgb(get_highest_layer(layer_state | state));
    return state;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (poc_power_should_sleep()) {
        for (uint8_t led = led_min; led < led_max && led < RGB_MATRIX_LED_COUNT; ++led) {
            RGB_MATRIX_INDICATOR_SET_COLOR(led, 0, 0, 0);
        }
        return true;
    }
    codex_micro_rgb_indicators(led_min, led_max);
    // 左上角切层键：按住期间点亮（LED 索引 0 = R1C1）。
    if (layer_key_pressed && get_highest_layer(layer_state | default_layer_state) == POC_CODEX_LAYER && POC_LAYER_KEY_LED >= led_min && POC_LAYER_KEY_LED < led_max) {
        RGB_MATRIX_INDICATOR_SET_COLOR(POC_LAYER_KEY_LED, 255, 255, 255);
    }
    return true;
}
#endif

#ifdef OLED_ENABLE
bool oled_task_user(void) {
    oled_controller_render();
    return true;
}
#endif
