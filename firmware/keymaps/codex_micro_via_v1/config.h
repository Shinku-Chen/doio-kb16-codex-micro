#pragma once

// Base identity is Codex Micro. The VIA identity (DOIO KB16-01) is selected at
// boot by the patched USB descriptor code; see firmware/qmk-core-patches.
#undef VENDOR_ID
#undef PRODUCT_ID
#undef DEVICE_VER
#undef MANUFACTURER
#undef PRODUCT
#define VENDOR_ID 0x303A
#define PRODUCT_ID 0x8360
#define DEVICE_VER 0x0100
#define MANUFACTURER "Work Louder"
#define PRODUCT "Codex Micro"

// Codex Micro vendor HID: report ID 6 plus a 63-byte report body.
#define RAW_USAGE_PAGE 0xFF00
#define RAW_USAGE_ID 0x01
#define RAW_INPUT_USAGE_ID 0x01
#define RAW_OUTPUT_USAGE_ID 0x02
#define RAW_REPORT_ID 0x06
#define RAW_REPORT_COUNT 63
#define RAW_EPSIZE 64

// Four stored pages; the runtime reads the Codex page from flash instead.
#define DYNAMIC_KEYMAP_LAYER_COUNT 4
#define POC_EEPROM_RESERVED_LAYER_SLOTS 5
#define DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR (DYNAMIC_KEYMAP_EEPROM_ADDR + POC_EEPROM_RESERVED_LAYER_SLOTS * MATRIX_ROWS * MATRIX_COLS * 2)
#define DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR (DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR + POC_EEPROM_RESERVED_LAYER_SLOTS * NUM_ENCODERS * 2 * 2)

// User EEPROM holds the layout stamp and the one-time four-layer migration.
#define EECONFIG_USER_DATA_SIZE 8

// PA10 is TIM1_CH3. Use its channel DMA request on DMA1 stream 6 so the
// OLED's I2C2 RX can retain DMA1 stream 5.
#define WS2812_PWM_CHANNEL 3
#define WS2812_PWM_DRIVER PWMD1
#define WS2812_PWM_DMA_STREAM STM32_DMA1_STREAM6
#define WS2812_PWM_DMA_EVENT TIM_DIER_CC3DE

// A calm fallback for fresh EEPROM; existing saved RGB settings remain intact.
#undef RGB_MATRIX_DEFAULT_MODE
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#define RGB_MATRIX_DEFAULT_HUE 170
#define RGB_MATRIX_DEFAULT_SAT 140
#define RGB_MATRIX_DEFAULT_VAL 120

#undef RGB_MATRIX_TIMEOUT
#define RGB_MATRIX_TIMEOUT 0
#undef OLED_TIMEOUT
#define OLED_TIMEOUT 0
#define RGB_MATRIX_SLEEP
#define OLED_LOGO_TIMEOUT 0
