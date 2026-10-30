#include QMK_KEYBOARD_H

#include "codex_micro_protocol.h"
#include "oled_icons.h"
#include "poc_config.h"
#include "poc_power.h"
#include "poc_mode.h"

#include "keymap_introspection.h"

enum layer_names {
    _VIA1 = POC_NORMAL_FIRST_LAYER,
    _VIA2,
    _VIA3,
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
#define POC_CODEX_L0_STAMP 0x304C4334UL

/* Layers 1-3 mirror the saved ordinary VIA layouts.
 * Layer 0 keeps native AG/ACT, joystick axes and encoder controls in flash.
 * All five spare key positions use VIA without a reserved long hold. */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_VIA1] = LAYOUT(
        KC_ESC, G(KC_W), LSG(KC_A), G(KC_BSPC), TO(1),
        G(KC_C), G(KC_V), LSG(KC_F), KC_ENT, TO(2),
        C(KC_D), G(KC_RBRC), C(KC_R), G(KC_F2), KC_MPLY,
        MT(MOD_LGUI, KC_MUTE), G(KC_LBRC), LAG(KC_R), G(KC_Z)),

    [_VIA2] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_PASTE, QK_MACRO_0, QK_MACRO_1, KC_ENT, TO(3),
        KC_F10, KC_F11, S(KC_F11), KC_F9, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_F8, KC_TRNS),

    [_VIA3] = LAYOUT(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, TO(0),
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
    [_VIA1] = {
        ENCODER_CCW_CW(KC_BRID, KC_BRIU),
        ENCODER_CCW_CW(KC_VOLD, KC_VOLU),
        ENCODER_CCW_CW(KC_MPRV, KC_MNXT),
    },
    [_VIA2] = {
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
        ENCODER_CCW_CW(KC_TRNS, KC_TRNS),
    },
    [_VIA3] = {
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

#define POC_LAYER_COUNT POC_VIA_LAYER_COUNT

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

uint8_t poc_keymap_base_layer(layer_state_t state) {
    layer_state_t normal_layers = ((1UL << POC_VIA_LAYER_COUNT) - 1) & ~(1UL << POC_CODEX_LAYER);
    return state & normal_layers ? POC_NORMAL_FIRST_LAYER : POC_CODEX_LAYER;
}

// Layers 1-3 use VIA data; layer 0 uses VIA only at the five custom positions.
// QMK's own keymap_key_to_keycode() is weak, so this replaces it wholesale.
uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key) {
    if (layer >= POC_VIA_LAYER_COUNT) {
        return KC_NO;
    }
    if (layer == POC_CODEX_LAYER) {
        if (key.row < MATRIX_ROWS && key.col < MATRIX_COLS) {
            if (poc_codex_custom_position(key)) {
                return poc_codex_custom_keycode(key);
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

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
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
    fresh = POC_CODEX_L0_STAMP;
    eeconfig_update_user_datablock(&fresh, sizeof(uint32_t), sizeof(fresh));
}

static uint16_t poc_remap_legacy_codex_keycode(uint16_t keycode) {
    if (IS_QK_LAYER_TAP(keycode) && QK_LAYER_TAP_GET_LAYER(keycode) == 4) {
        return (keycode & 0xF0FFU) | (POC_LEGACY_CODEX_LAYER << 8);
    }
    if (IS_QK_LAYER_MOD(keycode) && QK_LAYER_MOD_GET_LAYER(keycode) == 4) {
        return (keycode & 0xFE1FU) | (POC_LEGACY_CODEX_LAYER << 5);
    }
    bool layer_action = IS_QK_TO(keycode) || IS_QK_MOMENTARY(keycode) || IS_QK_DEF_LAYER(keycode) || IS_QK_PERSISTENT_DEF_LAYER(keycode) || IS_QK_TOGGLE_LAYER(keycode) || IS_QK_ONE_SHOT_LAYER(keycode) || IS_QK_LAYER_TAP_TOGGLE(keycode);
    if (layer_action && (keycode & 0x1FU) == 4) {
        return (keycode & 0xFFE0U) | POC_LEGACY_CODEX_LAYER;
    }
    return keycode;
}

static void poc_migrate_four_layer_layout(void) {
    uint32_t stamp = 0;
    eeconfig_read_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
    if (stamp == POC_FOUR_LAYER_STAMP || stamp == POC_CUSTOM_KEYS_STAMP || stamp == POC_CODEX_L0_STAMP) {
        return;
    }

    for (uint8_t layer = 0; layer < POC_LEGACY_CODEX_LAYER; ++layer) {
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
    if (stamp == POC_CUSTOM_KEYS_STAMP || stamp == POC_CODEX_L0_STAMP) {
        return;
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t column = 0; column < MATRIX_COLS; ++column) {
            if (poc_codex_custom_position((keypos_t){.row = row, .col = column})) {
                dynamic_keymap_set_keycode(POC_LEGACY_CODEX_LAYER, row, column, KC_NO);
            }
        }
    }
    stamp = POC_CUSTOM_KEYS_STAMP;
    eeconfig_update_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
}

static uint16_t poc_remap_shifted_layer_keycode(uint16_t keycode) {
    if (IS_QK_LAYER_TAP(keycode)) {
        uint8_t layer = QK_LAYER_TAP_GET_LAYER(keycode);
        return layer < POC_VIA_LAYER_COUNT ? (keycode & 0xF0FFU) | (((layer + 1) % POC_VIA_LAYER_COUNT) << 8) : keycode;
    }
    if (IS_QK_LAYER_MOD(keycode)) {
        uint8_t layer = QK_LAYER_MOD_GET_LAYER(keycode);
        return layer < POC_VIA_LAYER_COUNT ? (keycode & 0xFE1FU) | (((layer + 1) % POC_VIA_LAYER_COUNT) << 5) : keycode;
    }
    bool layer_action = IS_QK_TO(keycode) || IS_QK_MOMENTARY(keycode) || IS_QK_DEF_LAYER(keycode) || IS_QK_PERSISTENT_DEF_LAYER(keycode) || IS_QK_TOGGLE_LAYER(keycode) || IS_QK_ONE_SHOT_LAYER(keycode) || IS_QK_LAYER_TAP_TOGGLE(keycode);
    uint8_t layer = keycode & 0x1FU;
    return layer_action && layer < POC_VIA_LAYER_COUNT ? (keycode & 0xFFE0U) | ((layer + 1) % POC_VIA_LAYER_COUNT) : keycode;
}

static void poc_migrate_codex_l0_layout(void) {
    uint32_t stamp = 0;
    eeconfig_read_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
    if (stamp == POC_CODEX_L0_STAMP) {
        return;
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t column = 0; column < MATRIX_COLS; ++column) {
            uint16_t keycodes[POC_VIA_LAYER_COUNT];
            for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
                keycodes[layer] = dynamic_keymap_get_keycode(layer, row, column);
            }
            for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
                uint8_t source = (layer + POC_VIA_LAYER_COUNT - 1) % POC_VIA_LAYER_COUNT;
                dynamic_keymap_set_keycode(layer, row, column, poc_remap_shifted_layer_keycode(keycodes[source]));
            }
        }
    }
#ifdef ENCODER_MAP_ENABLE
    for (uint8_t encoder = 0; encoder < NUM_ENCODERS; ++encoder) {
        for (uint8_t direction = 0; direction < NUM_DIRECTIONS; ++direction) {
            uint16_t keycodes[POC_VIA_LAYER_COUNT];
            for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
                keycodes[layer] = dynamic_keymap_get_encoder(layer, encoder, direction != 0);
            }
            for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
                uint8_t source = (layer + POC_VIA_LAYER_COUNT - 1) % POC_VIA_LAYER_COUNT;
                dynamic_keymap_set_encoder(layer, encoder, direction != 0, poc_remap_shifted_layer_keycode(keycodes[source]));
            }
        }
    }
#endif
    stamp = POC_CODEX_L0_STAMP;
    eeconfig_update_user_datablock(&stamp, sizeof(uint32_t), sizeof(stamp));
}

#ifdef RGB_MATRIX_ENABLE
static void codex_rgb_init(void);
#endif

void keyboard_post_init_user(void) {
    poc_check_layout_stamp();
    poc_migrate_four_layer_layout();
    poc_migrate_codex_custom_keys();
    poc_migrate_codex_l0_layout();
    codex_micro_init();
    oled_controller_init();
#ifdef RGB_MATRIX_ENABLE
    codex_rgb_init();
#endif
    default_layer_set(1UL << POC_CODEX_LAYER);
    if (poc_mode_is_codex()) {
        layer_move(POC_CODEX_LAYER);
    } else {
        layer_move(POC_NORMAL_FIRST_LAYER);
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
    return true;
}
#endif

#ifdef OLED_ENABLE
bool oled_task_user(void) {
    oled_controller_render();
    return true;
}
#endif
