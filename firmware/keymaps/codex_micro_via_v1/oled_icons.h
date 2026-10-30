#pragma once

#include <stdbool.h>
#include <stdint.h>

#define OLED_POPUP_NORMAL_MS 1200
#define OLED_POPUP_ERROR_MS 2500
#define OLED_LAYER_SPLASH_MS 1000U
#define OLED_KEY_IDLE_MS 900U
#define OLED_MARGIN_X 6
#define OLED_MARGIN_Y 4
#define OLED_INPUT_LABEL_SIZE 39

typedef enum {
    OLED_INPUT_KEYBOARD,
    OLED_INPUT_MOUSE,
    OLED_INPUT_EXTRA,
    OLED_INPUT_SYSTEM,
    OLED_INPUT_COUNT,
} oled_input_source_t;

void oled_controller_init(void);
void oled_controller_note_input(oled_input_source_t source, const char *label, bool active);
void oled_controller_show_popup(const char *line1, const char *line2, uint16_t duration_ms);
void oled_controller_render(void);
void poc_oled_input_reset(void);
void poc_oled_note_keys(uint8_t mods, const uint8_t *data, uint8_t length, bool bitmap);
void poc_oled_note_mouse(uint8_t buttons, int16_t horizontal, int16_t vertical, int16_t wheel_vertical, int16_t wheel_horizontal);
void poc_oled_note_extra(uint16_t usage, bool consumer);
