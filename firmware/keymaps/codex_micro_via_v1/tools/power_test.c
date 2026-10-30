#include <assert.h>
#include <stdio.h>

#include "test_qmk.h"
#include "poc_config.h"
#include "poc_power.h"

layer_state_t layer_state;
layer_state_t default_layer_state = 1UL;
static bool suspended;
static uint32_t fake_timer;
static uint32_t input_idle;

uint32_t timer_read32(void) { return fake_timer; }
uint32_t last_input_activity_elapsed(void) { return input_idle; }
bool rgb_matrix_get_suspend_state(void) { return suspended; }
uint8_t get_highest_layer(layer_state_t state) {
    for (int layer = 31; layer >= 0; --layer) {
        if (state & (1UL << layer)) return (uint8_t)layer;
    }
    return 0;
}

static void reset_power(uint8_t layer) {
    layer_state = 1UL << layer;
    default_layer_state = 1UL;
    fake_timer = 0;
    input_idle = 0;
    suspended = false;
    poc_power_init();
}

static void test_idle_and_input(void) {
    for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
        reset_power(layer);
        assert(!poc_power_should_sleep());
        assert(!poc_power_should_oled_sleep());
        fake_timer = input_idle = POC_IDLE_SLEEP_MS - 1;
        assert(!poc_power_should_sleep());
        assert(!poc_power_should_oled_sleep());
        ++fake_timer;
        ++input_idle;
        assert(poc_power_should_sleep());
        assert(poc_power_should_oled_sleep());
        input_idle = 0;
        assert(!poc_power_should_sleep());
        assert(!poc_power_should_oled_sleep());
    }
    puts("PASS: all four layers share the exact one-hour idle boundary and input wakeup");
}

static void test_heartbeat_renewal(void) {
    for (uint8_t layer = 0; layer < POC_VIA_LAYER_COUNT; ++layer) {
        reset_power(layer);
        for (uint8_t hour = 1; hour <= 3; ++hour) {
            fake_timer = input_idle = hour * POC_IDLE_SLEEP_MS - 1;
            poc_power_note_host_activity();
            assert(!poc_power_should_sleep());
            assert(!poc_power_should_oled_sleep());
        }
        fake_timer += POC_IDLE_SLEEP_MS - 1;
        input_idle += POC_IDLE_SLEEP_MS - 1;
        assert(!poc_power_should_sleep());
        assert(!poc_power_should_oled_sleep());
        ++fake_timer;
        ++input_idle;
        assert(poc_power_should_sleep());
        assert(poc_power_should_oled_sleep());
        poc_power_note_host_activity();
        assert(!poc_power_should_sleep());
        assert(!poc_power_should_oled_sleep());
    }
    puts("PASS: every heartbeat or repeated packet renews the one-hour timer on VIA and Codex; silence expires it and a later packet wakes it");
}

static void test_host_off_priority(void) {
    reset_power(POC_CODEX_LAYER);
    poc_power_note_host_activity();
    poc_power_note_lighting(false);
    assert(poc_power_should_sleep());
    fake_timer = POC_IDLE_SLEEP_MS * 2;
    poc_power_note_host_activity();
    assert(poc_power_should_sleep());
    input_idle = 0;
    assert(poc_power_should_sleep());
    layer_state = 1UL << POC_NORMAL_FIRST_LAYER;
    assert(!poc_power_should_sleep());
    layer_state = 1UL << POC_CODEX_LAYER;
    assert(poc_power_should_sleep());
    poc_power_note_lighting(true);
    assert(!poc_power_should_sleep());
    puts("PASS: explicit all-off stays off across heartbeats and local input, applies only to Codex, and wakes on a host on-state");
}

static void test_suspend_and_wrap(void) {
    reset_power(POC_CODEX_LAYER);
    fake_timer = UINT32_MAX - 1000;
    poc_power_note_host_activity();
    fake_timer += POC_IDLE_SLEEP_MS - 1;
    input_idle = POC_IDLE_SLEEP_MS * 2;
    assert(!poc_power_should_sleep());
    assert(!poc_power_should_oled_sleep());
    ++fake_timer;
    assert(poc_power_should_sleep());
    assert(poc_power_should_oled_sleep());
    poc_power_note_host_activity();
    assert(!poc_power_should_sleep());
    assert(!poc_power_should_oled_sleep());
    suspended = true;
    assert(poc_power_should_sleep());
    assert(poc_power_should_oled_sleep());
    poc_power_note_host_activity();
    assert(poc_power_should_sleep());
    assert(poc_power_should_oled_sleep());
    suspended = false;
    assert(!poc_power_should_sleep());
    assert(!poc_power_should_oled_sleep());
    puts("PASS: USB suspend overrides heartbeat wake, resume preserves policy, and one-hour arithmetic survives timer wrap");
}

static void test_oled_power_independence(void) {
    reset_power(POC_CODEX_LAYER);
    poc_power_note_host_activity();
    poc_power_note_lighting(false);
    assert(poc_power_should_sleep());
    assert(!poc_power_should_oled_sleep());
    fake_timer = input_idle = POC_IDLE_SLEEP_MS;
    assert(poc_power_should_sleep());
    assert(poc_power_should_oled_sleep());
    poc_power_note_host_activity();
    assert(poc_power_should_sleep());
    assert(!poc_power_should_oled_sleep());
    fake_timer += POC_IDLE_SLEEP_MS;
    input_idle = POC_IDLE_SLEEP_MS * 2;
    assert(poc_power_should_oled_sleep());
    input_idle = 0;
    assert(poc_power_should_sleep());
    assert(!poc_power_should_oled_sleep());
    suspended = true;
    assert(poc_power_should_oled_sleep());
    puts("PASS: host all-off keeps RGB dark without hiding OLED; one-hour idle, heartbeats, input and USB suspend retain OLED power control");
}

int main(void) {
    test_idle_and_input();
    test_heartbeat_renewal();
    test_host_off_priority();
    test_suspend_and_wrap();
    test_oled_power_independence();
    return 0;
}
