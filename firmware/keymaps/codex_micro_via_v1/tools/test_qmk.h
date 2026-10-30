#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "quantum_keycodes.h"

typedef uint32_t layer_state_t;

extern layer_state_t layer_state;
extern layer_state_t default_layer_state;

#define RGB_MATRIX_LED_COUNT 16

uint8_t get_highest_layer(layer_state_t state);
uint32_t timer_read32(void);
uint16_t timer_read(void);
uint16_t timer_elapsed(uint16_t start);
uint32_t timer_elapsed32(uint32_t start);
void test_set_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);

#define PROGMEM
#define MATRIX_ROWS 4
#define MATRIX_COLS 5
#define NUM_ENCODERS 3
#define NUM_DIRECTIONS 2
#define ENCODER_MAP_ENABLE
#define KEYLOC_ENCODER_CW 0xFE
#define KEYLOC_ENCODER_CCW 0xFF
#define RGB_MATRIX_SOLID_COLOR 1

typedef struct { uint8_t row, col; } keypos_t;
typedef struct { struct { bool pressed; } event; } keyrecord_t;

void layer_move(uint8_t layer);
void default_layer_set(layer_state_t state);
uint16_t keycode_at_keymap_location_raw(uint8_t layer, uint8_t row, uint8_t col);
uint16_t keycode_at_encodermap_location_raw(uint8_t layer, uint8_t index, bool clockwise);
uint16_t dynamic_keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col);
void dynamic_keymap_set_keycode(uint8_t layer, uint8_t row, uint8_t col, uint16_t keycode);
uint32_t eeconfig_read_user_datablock(void *data, uint32_t offset, uint32_t length);
uint32_t eeconfig_update_user_datablock(const void *data, uint32_t offset, uint32_t length);
uint16_t dynamic_keymap_get_encoder(uint8_t layer, uint8_t index, bool clockwise);
void dynamic_keymap_set_encoder(uint8_t layer, uint8_t index, bool clockwise, uint16_t keycode);
uint8_t rgb_matrix_is_enabled(void);
uint8_t rgb_matrix_get_mode(void);
bool rgb_matrix_get_suspend_state(void);
uint32_t last_input_activity_elapsed(void);
void rgb_matrix_mode_noeeprom(uint8_t mode);
void rgb_matrix_enable_noeeprom(void);
void rgb_matrix_disable_noeeprom(void);
void oled_clear(void);
bool oled_on(void);
bool oled_off(void);
void oled_set_cursor(uint8_t col, uint8_t row);
void oled_write_ln(const char *text, bool invert);
void oled_write(const char *text, bool invert);
void oled_write_raw_P(const char *text, uint16_t length);
void oled_write_raw(const char *text, uint16_t length);

#define RGB_MATRIX_INDICATOR_SET_COLOR(index, red, green, blue) test_set_color((index), (red), (green), (blue))
