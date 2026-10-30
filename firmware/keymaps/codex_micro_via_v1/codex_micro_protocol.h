#pragma once

#ifdef CODEX_MICRO_HOST_TEST
#    include "test_qmk.h"
#else
#    include QMK_KEYBOARD_H
#endif

#define CODEX_MICRO_HOST_SLOT_COUNT 13
#define CODEX_MICRO_TASK_KEY_COUNT 6
#define CODEX_MICRO_REPORT_ID 0x06
#define CODEX_MICRO_REPORT_SIZE 64
#define CODEX_MICRO_REPORT_BODY_SIZE 63
#define CODEX_MICRO_FRAGMENT_SIZE 61
#define CODEX_MICRO_CHANNEL_RPC 2
#define CODEX_MICRO_VERSION "0.4.8-qmk"

typedef enum {
    CODEX_MICRO_ACTION_06,
    CODEX_MICRO_ACTION_07,
    CODEX_MICRO_ACTION_08,
    CODEX_MICRO_ACTION_09,
    CODEX_MICRO_ACTION_12,
    CODEX_MICRO_ACTION_11,
    CODEX_MICRO_ACTION_MIC,
    CODEX_MICRO_ACTION_COUNT,
} codex_micro_action_t;

typedef enum {
    CODEX_MICRO_JOYSTICK_UP,
    CODEX_MICRO_JOYSTICK_RIGHT,
    CODEX_MICRO_JOYSTICK_DOWN,
    CODEX_MICRO_JOYSTICK_LEFT,
    CODEX_MICRO_JOYSTICK_DIRECTION_COUNT,
} codex_micro_joystick_direction_t;

void codex_micro_init(void);
void codex_micro_task(void);
void codex_micro_send_agent_key(uint8_t slot, bool pressed);
void codex_micro_send_action_key(codex_micro_action_t action, bool pressed);
void codex_micro_send_joystick_direction(codex_micro_joystick_direction_t direction, bool pressed);
void codex_micro_send_encoder_turn(bool clockwise);
void codex_micro_send_encoder_press(bool pressed);
bool codex_micro_rgb_indicators(uint8_t led_min, uint8_t led_max);
void codex_micro_handle_report(uint8_t *data, uint8_t length);
const char *codex_micro_task_status(uint8_t slot);
