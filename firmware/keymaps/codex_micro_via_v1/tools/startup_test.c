#include <assert.h>
#include <stdio.h>

#include "test_qmk.h"
#include "config.h"
#include "poc_config.h"
#include "poc_mode.h"

typedef uint8_t oled_rotation_t;
static uint16_t startup_timer;
static uint32_t fake_timer;
static unsigned logo_calls;
static unsigned user_calls;
static bool user_result = true;
static unsigned codex_entries;
static poc_mode_t current_mode;
static bool via_window_started;
static bool via_seen;
static uint32_t via_window_start;

uint32_t timer_read32(void) { return fake_timer; }
uint16_t timer_read(void) { return (uint16_t)fake_timer; }
uint16_t timer_elapsed(uint16_t start) { return (uint16_t)(fake_timer - start); }
uint32_t timer_elapsed32(uint32_t start) { return fake_timer - start; }
static void render_logo(void) { ++logo_calls; }
static bool oled_task_user(void) { ++user_calls; return user_result; }
void poc_mode_enter_codex(void) { ++codex_entries; current_mode = POC_MODE_CODEX; }

#include "startup_under_test.h"

static void reset_window(uint32_t now) {
    fake_timer = now;
    current_mode = POC_MODE_VIA;
    via_window_started = false;
    via_seen = false;
    codex_entries = 0;
}

static void test_board_logo_bypass(void) {
    fake_timer = 0;
    assert(oled_init_kb(0) == 0);
    assert(oled_task_kb());
    assert(logo_calls == 0 && user_calls == 1);
    fake_timer = 5000;
    assert(oled_task_kb());
    assert(logo_calls == 0 && user_calls == 2);
    fake_timer = 65536;
    user_result = false;
    assert(!oled_task_kb());
    assert(logo_calls == 0 && user_calls == 3);
    puts("PASS: the actual KB16 board OLED hooks skip DOIO from the first frame, through five seconds and 16-bit timer wrap, and delegate to the user renderer");
}

static void test_waiting_state(void) {
    reset_window(0);
    assert(poc_mode_via_waiting());
    poc_mode_start_window();
    fake_timer = POC_VIA_WINDOW_MS - 1;
    poc_mode_task();
    assert(poc_mode_via_waiting() && codex_entries == 0);
    ++fake_timer;
    poc_mode_task();
    assert(!poc_mode_via_waiting() && codex_entries == 1);
    reset_window(UINT32_MAX - 1000);
    poc_mode_note_via_command();
    assert(!poc_mode_via_waiting());
    fake_timer += POC_VIA_WINDOW_MS + 1;
    poc_mode_task();
    assert(!poc_mode_via_waiting() && current_mode == POC_MODE_VIA && codex_entries == 0);
    puts("PASS: real mode-window functions display waiting until ten seconds or a VIA command, preserve VIA lock, and handle timer wrap");
}

int main(void) {
    test_board_logo_bypass();
    test_waiting_state();
    return 0;
}
