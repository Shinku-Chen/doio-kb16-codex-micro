#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codex_micro_protocol.h"
#include "oled_icons.h"
#include "poc_config.h"
#include "poc_power.h"
#include "glcdfont.c"
#include "oled_splash.h"

#define NKRO_ENABLE
#define MOUSE_ENABLE
#define EXTRAKEY_ENABLE
#define POC_OLED_INPUT
#define KEYBOARD_REPORT_SIZE 8
#define USB_PROTOCOL_BOOT 0
#define USB_PROTOCOL_REPORT 1
#define REPORT_ID_CONSUMER 4

typedef enum { USB_ENDPOINT_IN_KEYBOARD, USB_ENDPOINT_IN_SHARED, USB_ENDPOINT_IN_MOUSE } usb_endpoint_in_lut_t;
typedef struct { uint8_t mods, reserved, keys[6]; } report_keyboard_t;
typedef struct { uint8_t report_id, mods, bits[30]; } report_nkro_t;
typedef struct { uint8_t buttons; int16_t x, y, v, h; } report_mouse_t;
typedef struct __attribute__((packed)) { uint8_t report_id; uint16_t usage; } report_extra_t;

layer_state_t layer_state;
layer_state_t default_layer_state = 1UL;
static uint32_t fake_timer;
static uint8_t screen[512];
static unsigned screen_writes;
static bool display_active;
static bool rgb_suspended;
static uint32_t input_idle;
static bool via_waiting;
static bool enqueue_success = true;
static uint8_t usb_protocol = USB_PROTOCOL_REPORT;
static unsigned queued_reports;
static size_t queued_length;
static uint8_t queued_data[64];

static uint8_t usb_device_state_get_protocol(void) { return usb_protocol; }
static bool send_report(usb_endpoint_in_lut_t endpoint, void *report, size_t length) {
    assert(endpoint <= USB_ENDPOINT_IN_MOUSE && length <= sizeof(queued_data));
    ++queued_reports;
    queued_length = length;
    memcpy(queued_data, report, length);
    return enqueue_success;
}

#include "usb_input_under_test.h"

uint8_t get_highest_layer(layer_state_t state) {
    for (int layer = 31; layer >= 0; --layer) {
        if (state & (1UL << layer)) return (uint8_t)layer;
    }
    return 0;
}
uint32_t timer_read32(void) { return fake_timer; }
void test_set_color(uint8_t led, uint8_t red, uint8_t green, uint8_t blue) {
    (void)led; (void)red; (void)green; (void)blue;
}
void raw_hid_send(uint8_t *data, uint8_t length) {
    assert(length == 64 && data[0] == 6);
}
void oled_set_cursor(uint8_t col, uint8_t row) { assert(col == 0 && row == 0); }
bool oled_on(void) { display_active = true; return display_active; }
bool oled_off(void) { display_active = false; return display_active; }
bool rgb_matrix_get_suspend_state(void) { return rgb_suspended; }
uint32_t last_input_activity_elapsed(void) { return input_idle; }
bool poc_mode_via_waiting(void) { return via_waiting; }
void oled_write_raw(const char *data, uint16_t length) {
    assert(length == sizeof(screen));
    memcpy(screen, data, length);
    ++screen_writes;
}

static void feed(const char *json) {
    size_t length = strlen(json);
    for (size_t offset = 0; offset < length; offset += 61) {
        uint8_t report[64] = {6, 2};
        size_t chunk = length - offset < 61 ? length - offset : 61;
        report[2] = chunk;
        memcpy(report + 3, json + offset, chunk);
        codex_micro_handle_report(report, sizeof(report));
    }
}

static void reset_screen(uint8_t layer) {
    fake_timer = 0;
    layer_state = 1UL << layer;
    default_layer_state = 1UL;
    screen_writes = 0;
    display_active = false;
    rgb_suspended = false;
    input_idle = 0;
    via_waiting = false;
    queued_reports = 0;
    enqueue_success = true;
    usb_protocol = USB_PROTOCOL_REPORT;
    memset(screen, 0, sizeof(screen));
    codex_micro_init();
    oled_controller_init();
}

static void assert_margins(void) {
    for (uint8_t vertical = 0; vertical < 32; ++vertical) {
        for (uint8_t horizontal = 0; horizontal < 128; ++horizontal) {
            if (horizontal < OLED_MARGIN_X || horizontal >= 128 - OLED_MARGIN_X ||
                vertical < OLED_MARGIN_Y || vertical >= 32 - OLED_MARGIN_Y) {
                assert((screen[horizontal + (vertical / 8U) * 128U] & (1U << (vertical % 8U))) == 0);
            }
        }
    }
}

static void render(void) {
    oled_controller_render();
    assert_margins();
}

static void save_preview(const char *name) {
    const char *directory = getenv("KB16_OLED_PREVIEWS");
    if (directory == NULL) return;
    char path[512];
    int length = snprintf(path, sizeof(path), "%s/%s.oled", directory, name);
    assert(length > 0 && (size_t)length < sizeof(path));
    FILE *output = fopen(path, "wb");
    assert(output != NULL);
    assert(fwrite(screen, 1, sizeof(screen), output) == sizeof(screen));
    assert(fclose(output) == 0);
}

static void expected_text(uint8_t *expected, const char *text, uint8_t horizontal, uint8_t vertical, uint8_t scale) {
    for (size_t letter = 0; text[letter] != '\0'; ++letter) {
        for (uint8_t column = 0; column < 5; ++column) {
            uint8_t pixels = font[(uint8_t)text[letter] * 6U + column];
            for (uint8_t row = 0; row < 8; ++row) {
                if (!(pixels & (1U << row))) continue;
                for (uint8_t offset_y = 0; offset_y < scale; ++offset_y) {
                    for (uint8_t offset_x = 0; offset_x < scale; ++offset_x) {
                        size_t horizontal_pixel = horizontal + letter * 6U * scale + column * scale + offset_x;
                        size_t vertical_pixel = vertical + row * scale + offset_y;
                        assert(horizontal_pixel < 128 && vertical_pixel < 32);
                        expected[horizontal_pixel + (vertical_pixel / 8U) * 128U] |= 1U << (vertical_pixel % 8U);
                    }
                }
            }
        }
    }
}

static void assert_label(const char *label) {
    uint8_t expected[512] = {0};
    size_t length = strlen(label);
    if (length <= 19) {
        expected_text(expected, label, (128U - (length * 6U - 1U)) / 2U, 12, 1);
    } else {
        char first[20];
        memcpy(first, label, 19);
        first[19] = '\0';
        expected_text(expected, first, 7, 8, 1);
        expected_text(expected, label + 19, (128U - ((length - 19U) * 6U - 1U)) / 2U, 16, 1);
    }
    assert(memcmp(screen, expected, sizeof(expected)) == 0);
}

static void assert_digit(char digit) {
    uint8_t expected[512] = {0};
    char text[] = {digit, '\0'};
    expected_text(expected, text, 59, 9, 2);
    assert(memcmp(screen, expected, sizeof(expected)) == 0);
}

static void keyboard(uint8_t mods, uint8_t key) {
    report_keyboard_t report = {.mods = mods, .keys = {key}};
    send_keyboard(&report);
}

static void test_via_idle_digits(void) {
    for (uint8_t layer = 0; layer < 3; ++layer) {
        reset_screen(layer);
        render();
        assert_digit('1' + layer);
        char name[] = "via-1";
        name[4] += layer;
        save_preview(name);
        assert(screen_writes == 1);
        fake_timer = 5000;
        render();
        assert(screen_writes == 1);
    }
    layer_state = 0;
    default_layer_state = 1UL << 1;
    render();
    assert_digit('2');
    puts("PASS: VIA L0-L2 show centered 1/2/3 with safe margins, including default layers and unchanged-frame suppression");
}

static void test_codex_splash_and_tasks(void) {
    reset_screen(3);
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    save_preview("codex-logo");
    fake_timer = OLED_LAYER_SPLASH_MS - 1;
    render();
    assert(screen_writes == 1);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":16777215,\"e\":1},{\"id\":1,\"c\":3166206,\"e\":1},{\"id\":2,\"c\":65356,\"e\":1},{\"id\":3,\"c\":16739584,\"e\":1},{\"id\":4,\"c\":16711731,\"e\":1},{\"id\":5,\"c\":1193046,\"e\":1}]}");
    fake_timer = OLED_LAYER_SPLASH_MS;
    render();
    uint8_t expected[512] = {0};
    const char *const labels[] = {"1:IDLE", "2:RUN", "3:NEW", "4:WAIT", "5:ERR", "6:RUN"};
    for (uint8_t slot = 0; slot < 6; ++slot) {
        expected_text(expected, labels[slot], 8 + slot % 3 * 38, 7 + slot / 3 * 11, 1);
    }
    assert(memcmp(screen, expected, sizeof(expected)) == 0);
    save_preview("codex-tasks");
    unsigned before = screen_writes;
    oled_controller_show_popup("TASK 1", "REQUESTED", OLED_POPUP_NORMAL_MS);
    render();
    assert(screen_writes == before);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}");
    render();
    assert(screen_writes == before + 1);
    layer_state = 1;
    render();
    assert_digit('1');
    layer_state = 1UL << 3;
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    puts("PASS: Codex entry shows the old logo/CODEX then six live AG-derived task states without obsolete popups");
}

static void test_cindy_task_labels(void) {
    reset_screen(3);
    render();
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":5009407,\"b\":0.8,\"e\":4,\"s\":0.35},{\"id\":1,\"c\":16752640,\"b\":0.8,\"e\":4},{\"id\":2,\"c\":3524441,\"b\":0.8,\"e\":1},{\"id\":3,\"c\":16729402,\"b\":0.8,\"e\":4},{\"id\":4,\"e\":0},{\"id\":5,\"c\":13633036,\"b\":0.72,\"e\":4}]}");
    fake_timer = OLED_LAYER_SPLASH_MS;
    render();
    uint8_t expected[512] = {0};
    const char *const labels[] = {"1:RUN", "2:WAIT", "3:NEW", "4:ERR", "5:--", "6:ERR"};
    for (uint8_t slot = 0; slot < 6; ++slot) {
        expected_text(expected, labels[slot], 8 + slot % 3 * 38, 7 + slot / 3 * 11, 1);
    }
    assert(memcmp(screen, expected, sizeof(expected)) == 0);
    puts("PASS: OLED estimates host fixture states by color family; red previews are indistinguishable from error-colored tasks");
}

static void test_generic_task_labels(void) {
    static const uint32_t palette[] = {0xFFF0E6, 0x2F54EB, 0x31B54F, 0xE8A61C, 0xE9293E, 0x9933CC};
    const char *const labels[] = {"1:IDLE", "2:RUN", "3:NEW", "4:WAIT", "5:ERR", "6:OTHR"};
    reset_screen(3);
    render();
    for (uint8_t slot = 0; slot < 6; ++slot) {
        char json[160];
        snprintf(json, sizeof(json), "{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":%u,\"c\":%lu,\"e\":1}]}", slot, (unsigned long)palette[slot]);
        feed(json);
    }
    fake_timer = OLED_LAYER_SPLASH_MS;
    render();
    uint8_t expected[512] = {0};
    for (uint8_t slot = 0; slot < 6; ++slot) {
        expected_text(expected, labels[slot], 8 + slot % 3 * 38, 7 + slot / 3 * 11, 1);
    }
    assert(memcmp(screen, expected, sizeof(expected)) == 0);
    puts("PASS: OLED uses generic warm-white/blue/green/amber/red estimates and preserves OTHER for ambiguous hues");
}

static void test_keyboard_reports(void) {
    reset_screen(0);
    render();
    keyboard(8, 0);
    keyboard(8, 0x06);
    keyboard(8, 0);
    keyboard(0, 0);
    render();
    assert_label("GUI+C");
    save_preview("via-shortcut");
    fake_timer = OLED_KEY_IDLE_MS - 1;
    render();
    assert_label("GUI+C");
    fake_timer++;
    render();
    assert_digit('1');
    keyboard(3, 0x0C);
    render();
    assert_label("CTRL+SHIFT+I");
    fake_timer += 5000;
    render();
    assert_label("CTRL+SHIFT+I");
    keyboard(0, 0);
    report_nkro_t nkro = {.report_id = 6, .mods = 8};
    nkro.bits[0] = 1U << 6;
    send_nkro(&nkro);
    render();
    assert_label("GUI+C");
    memset(nkro.bits, 0, sizeof(nkro.bits));
    nkro.mods = 0;
    send_nkro(&nkro);
    keyboard(0, 0x45);
    render();
    assert_label("F12");
    keyboard(0, 0);
    keyboard(0, 0x73);
    render();
    assert_label("F24");
    keyboard(0, 0);
    keyboard(0, 0xA5);
    render();
    assert_label("0xA5");
    puts("PASS: actual 6KRO/NKRO reports show mapped chords, macro output, held keys and release-to-idle instead of raw QMK actions");
}

static void test_media_and_mouse_reports(void) {
    reset_screen(1);
    report_extra_t extra = {.report_id = REPORT_ID_CONSUMER, .usage = 0xE9};
    send_extra(&extra);
    render();
    assert_label("VOL UP");
    extra.usage = 0;
    send_extra(&extra);
    extra.usage = 0x70;
    send_extra(&extra);
    render();
    assert_label("BRIGHTNESS DOWN");
    extra.usage = 0;
    send_extra(&extra);
    extra.usage = 0xCD;
    send_extra(&extra);
    render();
    assert_label("PLAY/PAUSE");
    extra.usage = 0;
    send_extra(&extra);
    extra.report_id = 3;
    extra.usage = 0x81;
    send_extra(&extra);
    render();
    assert_label("POWER");
    extra.usage = 0;
    send_extra(&extra);
    report_mouse_t mouse = {.buttons = 1};
    send_mouse(&mouse);
    render();
    assert_label("BTN1");
    mouse.buttons = 0;
    mouse.v = 1;
    send_mouse(&mouse);
    render();
    assert_label("SCROLL UP");
    mouse.v = 0;
    mouse.x = -1;
    send_mouse(&mouse);
    render();
    assert_label("MOVE LEFT");
    fake_timer += OLED_KEY_IDLE_MS;
    render();
    assert_digit('2');
    puts("PASS: media/brightness/rotary, system, mouse buttons, wheels and cursor reports use actual emitted usages");
}

static void test_edges_switches_and_wrap(void) {
    reset_screen(0);
    report_keyboard_t report = {.mods = 255, .keys = {0x2A, 0x4B, 0x4E, 0x39, 0x47, 0x58}};
    send_keyboard(&report);
    render();
    assert_margins();
    save_preview("via-long-shortcut");
    unsigned before = screen_writes;
    poc_oled_note_keys(0, NULL, 10, true);
    oled_controller_note_input(OLED_INPUT_COUNT, "INVALID", true);
    render();
    assert(screen_writes == before);
    layer_state = 1UL << 2;
    render();
    assert_digit('3');
    keyboard(0, 0);
    keyboard(0, 0x04);
    render();
    assert_label("A");
    report_mouse_t mouse = {.v = -1};
    send_mouse(&mouse);
    fake_timer += 3000;
    render();
    assert_label("SCROLL DOWN");
    keyboard(0, 0);
    fake_timer += OLED_KEY_IDLE_MS;
    render();
    assert_digit('3');
    reset_screen(0);
    fake_timer = UINT32_MAX - 200;
    keyboard(0, 0x05);
    keyboard(0, 0);
    fake_timer = 100;
    render();
    assert_label("B");
    fake_timer = 700;
    render();
    assert_digit('1');
    reset_screen(0);
    layer_state = 0;
    default_layer_state = 1UL << 3;
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    keyboard(0, 0x06);
    render();
    assert_label("C");
    puts("PASS: all OLED frames keep margins, bound long chords, clear stale layer input and handle held sources/default layers/timer wrap");
}

static void test_usb_enqueue_boundary(void) {
    reset_screen(0);
    render();
    unsigned before = screen_writes;
    enqueue_success = false;
    keyboard(8, 0x06);
    report_nkro_t nkro = {.mods = 8, .bits = {1U << 6}};
    send_nkro(&nkro);
    report_mouse_t mouse = {.buttons = 1};
    send_mouse(&mouse);
    report_extra_t extra = {.report_id = REPORT_ID_CONSUMER, .usage = 0xE9};
    send_extra(&extra);
    render();
    assert(screen_writes == before && queued_reports == 4);
    enqueue_success = true;
    usb_protocol = USB_PROTOCOL_BOOT;
    keyboard(8, 0x06);
    assert(queued_length == 8 && queued_data[0] == 8 && queued_data[2] == 6);
    render();
    assert_label("GUI+C");
    puts("PASS: real ChibiOS send functions notify OLED only after successful queueing and preserve boot/report transport");
}

static void test_display_power(void) {
    reset_screen(0);
    render();
    assert(display_active);
    unsigned writes = screen_writes;
    fake_timer = 180000;
    render();
    assert(display_active && screen_writes == writes);
    assert_digit('1');
    fake_timer = input_idle = POC_IDLE_SLEEP_MS;
    render();
    assert(!display_active && screen_writes == writes);
    feed("{\"method\":\"device.status\",\"id\":910009}");
    render();
    assert(display_active && screen_writes == writes);
    rgb_suspended = true;
    render();
    assert(!display_active && screen_writes == writes);
    rgb_suspended = false;
    render();
    assert(display_active && screen_writes == writes);
    display_active = false;
    render();
    assert(display_active && screen_writes == writes);
    assert_digit('1');
    puts("PASS: OLED uses one-hour fallback, heartbeat wake and USB suspend/resume without needing a changed display frame");
}

static void test_codex_host_display_power(void) {
    reset_screen(POC_CODEX_LAYER);
    render();
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":3166206,\"b\":1,\"e\":4,\"s\":0}]}");
    render();
    assert(display_active);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}");
    render();
    assert(display_active && poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    feed("{\"method\":\"device.status\",\"id\":910010}");
    render();
    assert(display_active && poc_power_should_sleep());
    feed("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":0,\"b\":1,\"e\":3,\"s\":0.4}}}");
    render();
    assert(display_active);
    feed("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0}}}");
    render();
    assert(display_active && poc_power_should_sleep());
    layer_state = 1;
    render();
    assert(display_active);
    assert_digit('1');
    layer_state = 1UL << POC_CODEX_LAYER;
    render();
    assert(display_active && poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    puts("PASS: Codex OLED remains on across task all-off, ambient zero brightness, heartbeat and layer entry while RGB obeys host off");
}

static void test_extended_tasks_keep_display_bounded(void) {
    reset_screen(POC_CODEX_LAYER);
    render();
    fake_timer = 1000;
    render();
    assert(!codex_micro_has_task_lights());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    uint8_t expected[sizeof(screen)] = {0};
    const char *const labels[] = {"1:--", "2:--", "3:--", "4:--", "5:--", "6:--"};
    for (uint8_t slot = 0; slot < 6; ++slot) {
        expected_text(expected, labels[slot], 8 + slot % 3 * 38, 7 + slot / 3 * 11, 1);
    }
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":6,\"c\":5009407,\"b\":1,\"e\":1},{\"id\":12,\"c\":16711680,\"b\":1,\"e\":1}]}");
    render();
    assert(codex_micro_has_task_lights());
    assert(memcmp(expected, screen, sizeof(screen)) == 0);
    assert(strcmp(codex_micro_task_status(6), "RUN") == 0);
    assert(strcmp(codex_micro_task_status(12), "ERR") == 0);
    assert_margins();
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":6,\"e\":0}]}");
    render();
    assert(codex_micro_has_task_lights());
    assert(memcmp(expected, screen, sizeof(screen)) == 0);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":12,\"e\":0}]}");
    render();
    assert(display_active && !codex_micro_has_task_lights());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    puts("PASS: extended task-light storage leaves the six-cell OLED layout within its safe margins");
}

static void test_empty_codex_logo(void) {
    reset_screen(POC_CODEX_LAYER);
    render();
    fake_timer = 60000;
    render();
    assert(display_active && !codex_micro_has_task_lights());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":5009407,\"b\":0,\"e\":4,\"s\":0}]}");
    render();
    assert(display_active && codex_micro_has_task_lights() && poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) != 0);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":0,\"b\":1,\"e\":1}]}");
    render();
    assert(display_active && codex_micro_has_task_lights() && poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) != 0);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}");
    feed("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":5009407,\"b\":1,\"e\":4}}}");
    render();
    assert(display_active && !codex_micro_has_task_lights() && !poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    feed("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":0}}}");
    render();
    assert(display_active && poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    puts("PASS: no assigned task lights keeps the Codex logo, brightness zero/black/frozen tasks stay allocated, and ambient alone is not a task");
}

static void test_visible_codex_entry(void) {
    reset_screen(0);
    render();
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":5009407,\"b\":0,\"e\":1}]}");
    layer_state = 1UL << POC_CODEX_LAYER;
    keyboard(0, 0);
    rgb_suspended = true;
    fake_timer = 5000;
    render();
    assert(!display_active);
    rgb_suspended = false;
    render();
    assert(display_active && poc_power_should_sleep());
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    fake_timer += OLED_LAYER_SPLASH_MS - 1;
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    ++fake_timer;
    render();
    assert(display_active && memcmp(screen, codex_splash, sizeof(screen)) != 0);
    puts("PASS: Codex entry starts its full one-second logo at the first visible frame despite early input, host RGB off or USB suspend");
}

static void test_codex_custom_output(void) {
    reset_screen(POC_CODEX_LAYER);
    render();
    keyboard(8, 0x06);
    render();
    assert_label("GUI+C");
    fake_timer = 5000;
    render();
    assert_label("GUI+C");
    keyboard(0, 0);
    fake_timer += OLED_KEY_IDLE_MS - 1;
    render();
    assert_label("GUI+C");
    ++fake_timer;
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    feed("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":5009407,\"b\":1,\"e\":4}]}");
    render();
    uint8_t tasks[sizeof(screen)];
    memcpy(tasks, screen, sizeof(tasks));
    keyboard(0, 0x04);
    keyboard(0, 0);
    keyboard(0, 0x05);
    render();
    assert_label("B");
    keyboard(0, 0);
    fake_timer += OLED_KEY_IDLE_MS;
    render();
    assert(memcmp(screen, tasks, sizeof(screen)) == 0);
    report_extra_t extra = {.report_id = REPORT_ID_CONSUMER, .usage = 0xE9};
    send_extra(&extra);
    render();
    assert_label("VOL UP");
    extra.usage = 0;
    send_extra(&extra);
    fake_timer += OLED_KEY_IDLE_MS;
    render();
    assert(memcmp(screen, tasks, sizeof(screen)) == 0);
    layer_state = 1;
    render();
    assert_digit('1');
    layer_state = 1UL << POC_CODEX_LAYER;
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    puts("PASS: Codex custom HID output shows held chords, macro keys and media labels, then restores tasks or logo after 900 ms without stale layer input");
}

static void test_via_boot_waiting(void) {
    reset_screen(0);
    via_waiting = true;
    render();
    assert_label("VIA WAITTING");
    assert_margins();
    save_preview("via-waiting");
    fake_timer = POC_VIA_WINDOW_MS - 1;
    render();
    assert_label("VIA WAITTING");
    layer_state = 1UL << POC_CODEX_LAYER;
    render();
    assert_label("VIA WAITTING");
    via_waiting = false;
    layer_state = 1;
    render();
    assert_digit('1');
    layer_state = 1UL << POC_CODEX_LAYER;
    render();
    assert(memcmp(screen, codex_splash, sizeof(screen)) == 0);
    puts("PASS: the boot window shows exact centered VIA WAITTING with safe margins, then yields to VIA pages or the Codex splash");
}

int main(void) {
    test_via_idle_digits();
    test_codex_splash_and_tasks();
    test_cindy_task_labels();
    test_generic_task_labels();
    test_keyboard_reports();
    test_media_and_mouse_reports();
    test_edges_switches_and_wrap();
    test_usb_enqueue_boundary();
    test_display_power();
    test_extended_tasks_keep_display_bounded();
    test_codex_host_display_power();
    test_empty_codex_logo();
    test_visible_codex_entry();
    test_codex_custom_output();
    test_via_boot_waiting();
    return 0;
}
