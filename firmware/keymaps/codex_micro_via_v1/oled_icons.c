#include QMK_KEYBOARD_H

#include <stdio.h>
#include <string.h>

#include "codex_micro_protocol.h"
#include "oled_icons.h"
#include "poc_config.h"
#include "poc_mode.h"
#include "poc_power.h"
#include "glcdfont.c"
#include "oled_splash.h"

#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_FRAME_BYTES 512
#define OLED_TEXT_COLS 19

typedef struct {
    char label[OLED_INPUT_LABEL_SIZE];
    bool active;
} oled_input_t;

static oled_input_t inputs[OLED_INPUT_COUNT];
static uint8_t current_layer = 0xff;
static uint8_t recent_source;
static bool have_input;
static uint32_t layer_entered_at;
static uint32_t input_updated_at;
static uint8_t frame[OLED_FRAME_BYTES];
static uint8_t last_frame[OLED_FRAME_BYTES];
static bool frame_valid;

static void sync_layer(void) {
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);
    if (current_layer == layer) {
        return;
    }
    current_layer = layer;
    layer_entered_at = timer_read32();
    memset(inputs, 0, sizeof(inputs));
    have_input = false;
}

void oled_controller_init(void) {
    memset(inputs, 0, sizeof(inputs));
    current_layer = 0xff;
    recent_source = 0;
    have_input = false;
    frame_valid = false;
    layer_entered_at = timer_read32();
    input_updated_at = layer_entered_at;
    poc_oled_input_reset();
}

void oled_controller_note_input(oled_input_source_t source, const char *label, bool active) {
    if ((unsigned)source >= OLED_INPUT_COUNT) {
        return;
    }
    sync_layer();
    if (current_layer >= POC_CODEX_LAYER) {
        return;
    }
    bool was_active = inputs[source].active;
    inputs[source].active = active;
    if (label != NULL && label[0] != '\0') {
        snprintf(inputs[source].label, sizeof(inputs[source].label), "%s", label);
        recent_source = source;
        input_updated_at = timer_read32();
        have_input = true;
    } else if (was_active && !active) {
        input_updated_at = timer_read32();
    }
}

void oled_controller_show_popup(const char *line1, const char *line2, uint16_t duration_ms) {
    (void)line1;
    (void)line2;
    (void)duration_ms;
}

static void put_pixel(uint8_t horizontal, uint8_t vertical) {
    if (horizontal < OLED_MARGIN_X || horizontal >= OLED_WIDTH - OLED_MARGIN_X ||
        vertical < OLED_MARGIN_Y || vertical >= OLED_HEIGHT - OLED_MARGIN_Y) {
        return;
    }
    frame[horizontal + (vertical / 8U) * OLED_WIDTH] |= 1U << (vertical % 8U);
}

static void draw_text(const char *text, uint8_t horizontal, uint8_t vertical, uint8_t scale) {
    for (size_t letter = 0; text[letter] != '\0'; ++letter) {
        uint8_t character = (uint8_t)text[letter];
        if (character < 32 || character > 126) {
            character = '?';
        }
        for (uint8_t column = 0; column < 5; ++column) {
            uint8_t pixels = pgm_read_byte(&font[character * 6U + column]);
            for (uint8_t row = 0; row < 8; ++row) {
                if ((pixels & (1U << row)) == 0) {
                    continue;
                }
                for (uint8_t offset_y = 0; offset_y < scale; ++offset_y) {
                    for (uint8_t offset_x = 0; offset_x < scale; ++offset_x) {
                        put_pixel(horizontal + letter * 6U * scale + column * scale + offset_x,
                                  vertical + row * scale + offset_y);
                    }
                }
            }
        }
    }
}

static void draw_centered(const char *text, uint8_t vertical, uint8_t scale) {
    size_t length = strlen(text);
    uint8_t width = length == 0 ? 0 : (length * 6U - 1U) * scale;
    draw_text(text, (OLED_WIDTH - width) / 2U, vertical, scale);
}

static void draw_input(void) {
    char line[OLED_TEXT_COLS + 1];
    const char *label = inputs[recent_source].label;
    size_t length = strlen(label);
    size_t first = length > OLED_TEXT_COLS ? OLED_TEXT_COLS : length;
    memcpy(line, label, first);
    line[first] = '\0';
    draw_centered(line, length > OLED_TEXT_COLS ? 8 : 12, 1);
    if (length > OLED_TEXT_COLS) {
        draw_centered(label + first, 16, 1);
    }
}

static void draw_tasks(void) {
    for (uint8_t slot = 0; slot < CODEX_MICRO_TASK_KEY_COUNT; ++slot) {
        const char *status = codex_micro_task_status(slot);
        if (strcmp(status, "OTHER") == 0) {
            status = "OTHR";
        }
        char label[7];
        snprintf(label, sizeof(label), "%u:%.4s", slot + 1U, status);
        draw_text(label, 8U + (slot % 3U) * 38U, 7U + (slot / 3U) * 11U, 1);
    }
}

void oled_controller_render(void) {
    if (poc_power_should_sleep()) {
        oled_off();
        return;
    }
    oled_on();
    sync_layer();
    memset(frame, 0, sizeof(frame));
    uint32_t now = timer_read32();
    if (poc_mode_via_waiting()) {
        draw_centered("VIA WAITTING", 12, 1);
    } else if (current_layer == POC_CODEX_LAYER) {
        if (now - layer_entered_at < OLED_LAYER_SPLASH_MS) {
            memcpy_P(frame, codex_splash, sizeof(frame));
        } else {
            draw_tasks();
        }
    } else if (current_layer < POC_NORMAL_LAYER_COUNT) {
        bool active = false;
        for (uint8_t source = 0; source < OLED_INPUT_COUNT; ++source) {
            active |= inputs[source].active;
        }
        if (have_input && (active || now - input_updated_at < OLED_KEY_IDLE_MS)) {
            draw_input();
        } else {
            char label[] = {(char)('1' + current_layer), '\0'};
            draw_centered(label, 9, 2);
        }
    }
    for (uint8_t vertical = 0; vertical < OLED_HEIGHT; ++vertical) {
        for (uint8_t horizontal = 0; horizontal < OLED_WIDTH; ++horizontal) {
            if (horizontal < OLED_MARGIN_X || horizontal >= OLED_WIDTH - OLED_MARGIN_X ||
                vertical < OLED_MARGIN_Y || vertical >= OLED_HEIGHT - OLED_MARGIN_Y) {
                frame[horizontal + (vertical / 8U) * OLED_WIDTH] &= (uint8_t)~(1U << (vertical % 8U));
            }
        }
    }
    if (!frame_valid || memcmp(frame, last_frame, sizeof(frame)) != 0) {
        oled_set_cursor(0, 0);
        oled_write_raw((const char *)frame, sizeof(frame));
        memcpy(last_frame, frame, sizeof(frame));
        frame_valid = true;
    }
}
