#ifdef CODEX_MICRO_HOST_TEST
#    include "test_qmk.h"
#else
#    include QMK_KEYBOARD_H
#endif

#include "poc_config.h"
#include "poc_power.h"

static uint32_t host_received_at;
static bool host_activity_seen;
static bool host_lighting_seen;
static bool host_lighting_on;

void poc_power_init(void) {
    host_received_at = timer_read32();
    host_activity_seen = false;
    host_lighting_seen = false;
    host_lighting_on = false;
}

void poc_power_note_host_activity(void) {
    host_received_at = timer_read32();
    host_activity_seen = true;
}

void poc_power_note_lighting(bool on) {
    host_lighting_seen = true;
    host_lighting_on = on;
}

bool poc_power_should_sleep(void) {
    if (rgb_matrix_get_suspend_state()) {
        return true;
    }
    uint32_t idle = last_input_activity_elapsed();
    if (get_highest_layer(layer_state | default_layer_state) == POC_CODEX_LAYER) {
        if (host_lighting_seen && !host_lighting_on) {
            return true;
        }
    }
    uint32_t host_idle = timer_read32() - host_received_at;
    if (host_activity_seen && host_idle < idle) {
        idle = host_idle;
    }
    return idle >= POC_IDLE_SLEEP_MS;
}
