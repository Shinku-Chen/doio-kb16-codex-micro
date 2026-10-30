#include "codex_micro_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oled_icons.h"
#include "poc_config.h"
#include "poc_power.h"
#include "raw_hid.h"

#define CODEX_MICRO_RX_BUFFER_SIZE 2048
#define CODEX_MICRO_OBJECT_SIZE 256
#define CODEX_MICRO_LIGHT_SCALE 1000U
#define CODEX_MICRO_ANIMATION_CYCLE 16384000UL
#define CODEX_MICRO_DEFAULT_SPEED 400U
#define CODEX_MICRO_NEUTRAL_SATURATION_PERCENT 12U

typedef enum {
    CODEX_MICRO_EFFECT_OFF = 0,
    CODEX_MICRO_EFFECT_SOLID = 1,
    CODEX_MICRO_EFFECT_SNAKE = 2,
    CODEX_MICRO_EFFECT_RAINBOW = 3,
    CODEX_MICRO_EFFECT_BREATH = 4,
    CODEX_MICRO_EFFECT_GRADIENT = 5,
    CODEX_MICRO_EFFECT_SHALLOW_BREATH = 6,
} codex_micro_effect_t;

typedef enum {
    CODEX_MICRO_COLOR_DARK,
    CODEX_MICRO_COLOR_NEUTRAL,
    CODEX_MICRO_COLOR_RED,
    CODEX_MICRO_COLOR_AMBER,
    CODEX_MICRO_COLOR_GREEN,
    CODEX_MICRO_COLOR_TEAL,
    CODEX_MICRO_COLOR_CYAN,
    CODEX_MICRO_COLOR_BLUE,
    CODEX_MICRO_COLOR_OTHER,
} codex_micro_color_family_t;

typedef struct {
    uint32_t             color;
    uint32_t             animation_clock;
    uint32_t             clock_updated_at;
    uint16_t             brightness;
    uint16_t             speed;
    codex_micro_effect_t effect;
    bool                 assigned;
} codex_micro_light_t;

static codex_micro_light_t thread_lights[CODEX_MICRO_HOST_SLOT_COUNT];
static codex_micro_light_t ambient_light;
static char                rpc_buffer[CODEX_MICRO_RX_BUFFER_SIZE];
static size_t              rpc_length;
static bool                host_seen;
static uint8_t             sent_press_mask;
static uint8_t             sent_action_mask;
static bool                sent_encoder_press;
static uint8_t             joystick_held_mask;
static uint8_t             joystick_press_order[CODEX_MICRO_JOYSTICK_DIRECTION_COUNT];
static uint8_t             joystick_press_count;
static uint8_t             act_pressed_mask;

static const char   *const action_key_names[CODEX_MICRO_ACTION_COUNT] = {"ACT06", "ACT07", "ACT08", "ACT09", "ACT12", "ACT11", "ACT10"};
static const char   *const joystick_angles[CODEX_MICRO_JOYSTICK_DIRECTION_COUNT] = {"0.75", "0.00", "0.25", "0.50"};
/* 灯槽 0–5 对应原生 AG，扩展槽 6–12 对应 ACT06–ACT12 的物理灯；不改变输入上报。 */
static const uint8_t task_led_index[CODEX_MICRO_HOST_SLOT_COUNT] = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15};

/* ACT06-09 = R3C1-R3C4, ACT10/11/12 = R4C2-R4C4. */
static const uint8_t act_led_index[CODEX_MICRO_ACTION_COUNT] = {8, 9, 10, 11, 15, 14, 13};
static const uint8_t ambient_led_index[] = {12, 13, 14, 15};

static void send_json(const char *json) {
    size_t json_length = strlen(json);
    size_t total       = json_length + 1;
    size_t offset      = 0;

    while (offset < total) {
        uint8_t report[CODEX_MICRO_REPORT_SIZE] = {0};
        size_t  chunk = total - offset;
        if (chunk > CODEX_MICRO_FRAGMENT_SIZE) {
            chunk = CODEX_MICRO_FRAGMENT_SIZE;
        }

        report[0] = CODEX_MICRO_REPORT_ID;
        report[1] = CODEX_MICRO_CHANNEL_RPC;
        report[2] = (uint8_t)chunk;
        for (size_t index = 0; index < chunk; ++index) {
            size_t source = offset + index;
            report[3 + index] = source < json_length ? (uint8_t)json[source] : (uint8_t)'\n';
        }

        raw_hid_send(report, sizeof(report));
        offset += chunk;
    }
}

static void show_no_link(void) {
    oled_controller_show_popup("NO LINK", "CODEX MICRO", OLED_POPUP_ERROR_MS);
}

static void send_hid_key_event(const char *key, uint8_t action) {
    char message[128];
    int  length = snprintf(message, sizeof(message), "{\"method\":\"v.oai.hid\",\"params\":{\"k\":\"%s\",\"act\":%u}}", key, action);
    if (length > 0 && (size_t)length < sizeof(message)) {
        send_json(message);
    }
}

static void send_joystick_position(codex_micro_joystick_direction_t direction, bool active) {
    const char *angle = direction < CODEX_MICRO_JOYSTICK_DIRECTION_COUNT ? joystick_angles[direction] : "0.00";
    char        message[128];
    int         length = snprintf(message, sizeof(message), "{\"method\":\"v.oai.rad\",\"params\":{\"a\":%s,\"d\":%u}}", angle, active ? 1U : 0U);
    if (length > 0 && (size_t)length < sizeof(message)) {
        send_json(message);
    }
}

static const char *find_top_level_value(const char *json, const char *key) {
    size_t key_length = strlen(key);
    int    depth      = 0;

    for (const char *cursor = json; *cursor != '\0'; ++cursor) {
        if (*cursor == '{') {
            ++depth;
            continue;
        }
        if (*cursor == '}') {
            --depth;
            if (depth == 0) {
                return NULL;
            }
            continue;
        }
        if (*cursor != '"') {
            continue;
        }

        const char *start   = cursor + 1;
        const char *closing = start;
        bool        escaped = false;
        while (*closing != '\0') {
            if (!escaped && *closing == '"') {
                break;
            }
            if (!escaped && *closing == '\\') {
                escaped = true;
            } else {
                escaped = false;
            }
            ++closing;
        }
        if (*closing == '\0') {
            return NULL;
        }

        if (depth == 1 && (size_t)(closing - start) == key_length && memcmp(start, key, key_length) == 0) {
            const char *value = closing + 1;
            while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') {
                ++value;
            }
            if (*value != ':') {
                return NULL;
            }
            ++value;
            while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') {
                ++value;
            }
            return value;
        }
        cursor = closing;
    }
    return NULL;
}

static bool copy_json_string(const char *value, char *target, size_t target_size) {
    if (value == NULL || *value != '"' || target_size == 0) {
        return false;
    }

    size_t output  = 0;
    bool   escaped = false;
    for (const char *cursor = value + 1; *cursor != '\0'; ++cursor) {
        if (!escaped && *cursor == '"') {
            target[output] = '\0';
            return true;
        }
        if (output + 1 >= target_size) {
            return false;
        }
        if (!escaped && *cursor == '\\') {
            escaped = true;
            continue;
        }
        target[output++] = *cursor;
        escaped          = false;
    }
    return false;
}

static bool copy_json_token(const char *value, char *target, size_t target_size) {
    if (value == NULL || target_size == 0) {
        return false;
    }

    const char *end = value;
    if (*value == '"') {
        bool escaped = false;
        ++end;
        while (*end != '\0') {
            if (!escaped && *end == '"') {
                ++end;
                break;
            }
            if (!escaped && *end == '\\') {
                escaped = true;
            } else {
                escaped = false;
            }
            ++end;
        }
    } else {
        while (*end != '\0' && *end != ',' && *end != '}' && *end != '\r' && *end != '\n') {
            ++end;
        }
        while (end > value && (end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
    }

    size_t length = (size_t)(end - value);
    if (length == 0 || length >= target_size) {
        return false;
    }
    memcpy(target, value, length);
    target[length] = '\0';
    return true;
}

static bool parse_u32(const char *value, uint32_t *result) {
    if (value == NULL || result == NULL || *value < '0' || *value > '9') {
        return false;
    }

    uint32_t parsed = 0;
    while (*value >= '0' && *value <= '9') {
        parsed = parsed * 10U + (uint32_t)(*value - '0');
        ++value;
    }
    *result = parsed;
    return true;
}

static bool parse_light_level(const char *value, uint16_t *result) {
    if (value == NULL || result == NULL || *value < '0' || *value > '9') {
        return false;
    }

    uint8_t whole = 0;
    while (*value >= '0' && *value <= '9') {
        if (whole == 0 && *value != '0') {
            whole = 1;
        }
        ++value;
    }

    if (whole >= 1U) {
        *result = CODEX_MICRO_LIGHT_SCALE;
        return true;
    }

    uint32_t fraction = 0;
    uint32_t scale    = 1;
    if (*value == '.') {
        ++value;
        for (uint8_t digits = 0; digits < 4 && *value >= '0' && *value <= '9'; ++digits, ++value) {
            fraction = fraction * 10U + (uint32_t)(*value - '0');
            scale *= 10U;
        }
    }
    *result = (uint16_t)((fraction * CODEX_MICRO_LIGHT_SCALE + scale / 2U) / scale);
    return true;
}

static bool parse_light_effect(const char *value, codex_micro_effect_t *result) {
    uint32_t numeric;
    if (parse_u32(value, &numeric)) {
        if (numeric > CODEX_MICRO_EFFECT_SHALLOW_BREATH) {
            return false;
        }
        *result = (codex_micro_effect_t)numeric;
        return true;
    }

    char effect[16];
    if (!copy_json_string(value, effect, sizeof(effect))) {
        return false;
    }
    static const char *const effect_names[] = {"off", "solid", "snake", "rainbow", "breath", "gradient", "shallowBreath"};
    for (uint8_t index = 0; index <= CODEX_MICRO_EFFECT_SHALLOW_BREATH; ++index) {
        if (strcmp(effect, effect_names[index]) == 0) {
            *result = (codex_micro_effect_t)index;
            return true;
        }
    }
    return false;
}

static void advance_light_clock(codex_micro_light_t *light) {
    uint32_t now = timer_read32();
    uint32_t elapsed = now - light->clock_updated_at;
    light->clock_updated_at = now;
    if (elapsed == 0 || light->speed == 0) {
        return;
    }
    uint32_t advance = (uint32_t)(((uint64_t)elapsed * light->speed * 30U) % CODEX_MICRO_ANIMATION_CYCLE);
    light->animation_clock = (light->animation_clock + advance) % CODEX_MICRO_ANIMATION_CYCLE;
}

static void init_light(codex_micro_light_t *light) {
    memset(light, 0, sizeof(*light));
    light->brightness = CODEX_MICRO_LIGHT_SCALE;
    light->speed = CODEX_MICRO_DEFAULT_SPEED;
    light->clock_updated_at = timer_read32();
}

static const char *find_light_value(const char *object, const char *short_name, const char *long_name) {
    const char *value = find_top_level_value(object, short_name);
    return value != NULL ? value : find_top_level_value(object, long_name);
}

static void mark_host_seen(void) {
    if (host_seen) {
        return;
    }
    host_seen = true;
    oled_controller_show_popup("CODEX MICRO", "CONNECTED", OLED_POPUP_NORMAL_MS);
}

static void send_result(const char *id, const char *result) {
    if (id == NULL) {
        return;
    }
    char response[CODEX_MICRO_OBJECT_SIZE];
    int  length = snprintf(response, sizeof(response), "{\"id\":%s,\"result\":%s}", id, result);
    if (length > 0 && (size_t)length < sizeof(response)) {
        send_json(response);
    }
}

static void send_method_error(const char *id) {
    if (id == NULL) {
        return;
    }
    char response[CODEX_MICRO_OBJECT_SIZE];
    int  length = snprintf(response, sizeof(response), "{\"id\":%s,\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}", id);
    if (length > 0 && (size_t)length < sizeof(response)) {
        send_json(response);
    }
}

static bool update_light(codex_micro_light_t *light, const char *object) {
    if (object == NULL || *object != '{') {
        return false;
    }
    bool updated = false;
    uint32_t color;
    uint16_t brightness;
    uint16_t speed;
    codex_micro_effect_t effect;
    advance_light_clock(light);
    if (parse_u32(find_light_value(object, "c", "color"), &color)) {
        updated = true;
        light->color = color & 0xFFFFFFU;
    }
    if (parse_light_level(find_light_value(object, "b", "brightness"), &brightness)) {
        updated = true;
        light->brightness = brightness;
    }
    if (parse_light_level(find_light_value(object, "s", "speed"), &speed)) {
        updated = true;
        light->speed = speed;
    }
    const char *effect_value = find_light_value(object, "e", "effect");
    if (parse_light_effect(effect_value, &effect)) {
        updated = true;
        light->effect   = effect;
        light->assigned = effect != CODEX_MICRO_EFFECT_OFF;
    }
    return updated;
}

static bool light_has_output(const codex_micro_light_t *light) {
    return light->assigned && light->effect != CODEX_MICRO_EFFECT_OFF && light->brightness > 0 &&
           (light->color != 0 || light->effect == CODEX_MICRO_EFFECT_RAINBOW || light->effect == CODEX_MICRO_EFFECT_GRADIENT);
}

static void note_lighting_power(bool updated) {
    if (!updated) {
        return;
    }
    bool on = light_has_output(&ambient_light);
    for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        on |= light_has_output(&thread_lights[slot]);
    }
    poc_power_note_lighting(on);
}

static bool update_thread_lighting(const char *params) {
    if (params == NULL || *params != '[') {
        return false;
    }

    bool updated = false;
    const char *cursor = params;
    while (*cursor != '\0' && *cursor != ']') {
        if (*cursor != '{') {
            ++cursor;
            continue;
        }

        const char *start     = cursor;
        int         depth     = 0;
        bool        in_string = false;
        bool        escaped   = false;
        do {
            if (in_string) {
                if (!escaped && *cursor == '"') {
                    in_string = false;
                }
                if (!escaped && *cursor == '\\') {
                    escaped = true;
                } else {
                    escaped = false;
                }
            } else if (*cursor == '"') {
                in_string = true;
            } else if (*cursor == '{') {
                ++depth;
            } else if (*cursor == '}') {
                --depth;
            }
            ++cursor;
        } while (*cursor != '\0' && depth > 0);

        size_t length = (size_t)(cursor - start);
        if (depth == 0 && length < CODEX_MICRO_OBJECT_SIZE) {
            char object[CODEX_MICRO_OBJECT_SIZE];
            memcpy(object, start, length);
            object[length] = '\0';
            uint32_t id;
            if (parse_u32(find_top_level_value(object, "id"), &id) && id < CODEX_MICRO_HOST_SLOT_COUNT) {
                updated |= update_light(&thread_lights[id], object);
            }
        }
    }
    return updated;
}

static void handle_request(const char *json) {
    char method[32];
    char id[48];
    bool has_method = copy_json_string(find_top_level_value(json, "method"), method, sizeof(method));
    bool has_id     = copy_json_token(find_top_level_value(json, "id"), id, sizeof(id));
    if (!has_method) {
        return;
    }

    poc_power_note_host_activity();
    mark_host_seen();
    if (strcmp(method, "sys.version") == 0) {
        send_result(has_id ? id : NULL, "{\"version\":\"" CODEX_MICRO_VERSION "\"}");
    } else if (strcmp(method, "device.status") == 0) {
        uint8_t active_layer = get_highest_layer(layer_state | default_layer_state);
        uint8_t host_layer = active_layer == POC_CODEX_LAYER ? 1 : active_layer + 2;
        char status[160];
        snprintf(status, sizeof(status), "{\"version\":\"" CODEX_MICRO_VERSION "\",\"profile_index\":0,\"layer_index\":%u,\"battery\":100,\"is_charging\":false}", host_layer);
        send_result(has_id ? id : NULL, status);
    } else if (strcmp(method, "v.oai.thstatus") == 0) {
        note_lighting_power(update_thread_lighting(find_top_level_value(json, "params")));
        send_result(has_id ? id : NULL, "{\"ok\":true}");
    } else if (strcmp(method, "v.oai.rgbcfg") == 0 || strcmp(method, "lights.preview") == 0) {
        const char *params = find_top_level_value(json, "params");
        if (params != NULL && *params == '{') {
            note_lighting_power(update_light(&ambient_light, find_top_level_value(params, strcmp(method, "v.oai.rgbcfg") == 0 ? "ambient" : "underglow")));
        }
        send_result(has_id ? id : NULL, "{\"ok\":true}");
    } else if (strcmp(method, "host.focused_app") == 0) {
        send_result(has_id ? id : NULL, "{\"ok\":true}");
    } else {
        send_method_error(has_id ? id : NULL);
    }
}

static void process_rpc_buffer(void) {
    while (rpc_length > 0) {
        size_t start = 0;
        while (start < rpc_length && rpc_buffer[start] != '{') {
            ++start;
        }
        if (start == rpc_length) {
            rpc_length = 0;
            return;
        }
        if (start > 0) {
            memmove(rpc_buffer, rpc_buffer + start, rpc_length - start);
            rpc_length -= start;
        }

        int  depth     = 0;
        bool in_string = false;
        bool escaped   = false;
        for (size_t index = 0; index < rpc_length; ++index) {
            char value = rpc_buffer[index];
            if (in_string) {
                if (!escaped && value == '"') {
                    in_string = false;
                }
                if (!escaped && value == '\\') {
                    escaped = true;
                } else {
                    escaped = false;
                }
                continue;
            }
            if (value == '"') {
                in_string = true;
            } else if (value == '{') {
                ++depth;
            } else if (value == '}') {
                --depth;
                if (depth == 0) {
                    size_t object_length = index + 1;
                    char   saved         = rpc_buffer[object_length];
                    rpc_buffer[object_length] = '\0';
                    handle_request(rpc_buffer);
                    rpc_buffer[object_length] = saved;
                    memmove(rpc_buffer, rpc_buffer + object_length, rpc_length - object_length);
                    rpc_length -= object_length;
                    goto next_object;
                }
            }
        }
        return;

    next_object:
        continue;
    }
}

void codex_micro_init(void) {
    poc_power_init();
    for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        init_light(&thread_lights[slot]);
    }
    init_light(&ambient_light);
    rpc_length      = 0;
    host_seen       = false;
    sent_press_mask = 0;
    sent_action_mask = 0;
    sent_encoder_press = false;
    joystick_held_mask = 0;
    joystick_press_count = 0;
    act_pressed_mask = 0;
    // 默认不点亮底光：ACT 键和空位平时黑，只在按下时亮；AG 键按任务状态亮。
}

void codex_micro_task(void) {
    process_rpc_buffer();
}

void codex_micro_send_agent_key(uint8_t slot, bool pressed) {
    if (slot >= CODEX_MICRO_TASK_KEY_COUNT) {
        return;
    }

    uint8_t mask = (uint8_t)(1U << slot);
    if (pressed) {
        sent_press_mask |= mask;
    } else {
        if ((sent_press_mask & mask) == 0) {
            return;
        }
        sent_press_mask &= (uint8_t)~mask;
    }

    char message[128];
    int  length = snprintf(message, sizeof(message), "{\"method\":\"v.oai.hid\",\"params\":{\"k\":\"AG%02u\",\"act\":%u,\"ag\":%u}}", slot, pressed ? 1U : 0U, slot);
    if (length > 0 && (size_t)length < sizeof(message)) {
        send_json(message);
    }

    if (pressed) {
        char label[] = "TASK 1";
        label[5]     = (char)('1' + slot);
        oled_controller_show_popup(label, "REQUESTED", OLED_POPUP_NORMAL_MS);
    }
}

void codex_micro_send_action_key(codex_micro_action_t action, bool pressed) {
    if (action >= CODEX_MICRO_ACTION_COUNT) {
        return;
    }

    // 按下时点亮对应键，跟宿主是否连上无关。
    uint8_t mask = (uint8_t)(1U << action);
    if (pressed) {
        act_pressed_mask |= mask;
    } else {
        act_pressed_mask &= (uint8_t)~mask;
    }
    if (pressed) {
        if (!host_seen) {
            show_no_link();
            return;
        }
        sent_action_mask |= mask;
    } else {
        if ((sent_action_mask & mask) == 0) {
            return;
        }
        sent_action_mask &= (uint8_t)~mask;
    }

    send_hid_key_event(action_key_names[action], pressed ? 1U : 0U);
}

void codex_micro_send_joystick_direction(codex_micro_joystick_direction_t direction, bool pressed) {
    if (direction >= CODEX_MICRO_JOYSTICK_DIRECTION_COUNT) {
        return;
    }

    uint8_t mask = (uint8_t)(1U << direction);
    if (pressed) {
        if ((joystick_held_mask & mask) != 0) {
            return;
        }
        if (!host_seen) {
            show_no_link();
            return;
        }
        joystick_held_mask |= mask;
        joystick_press_order[joystick_press_count++] = (uint8_t)direction;
        send_joystick_position(direction, true);
        return;
    }

    if ((joystick_held_mask & mask) == 0) {
        return;
    }

    bool was_active = joystick_press_count > 0 && joystick_press_order[joystick_press_count - 1] == (uint8_t)direction;
    for (uint8_t index = 0; index < joystick_press_count; ++index) {
        if (joystick_press_order[index] == (uint8_t)direction) {
            memmove(&joystick_press_order[index], &joystick_press_order[index + 1], joystick_press_count - index - 1);
            --joystick_press_count;
            break;
        }
    }
    joystick_held_mask &= (uint8_t)~mask;

    if (was_active) {
        if (joystick_press_count > 0) {
            send_joystick_position((codex_micro_joystick_direction_t)joystick_press_order[joystick_press_count - 1], true);
        } else {
            send_joystick_position(direction, false);
        }
    }
}

void codex_micro_send_encoder_turn(bool clockwise) {
    if (!host_seen) {
        return;
    }
    send_hid_key_event(clockwise ? "ENC_CW" : "ENC_CC", 2);
}

void codex_micro_send_encoder_press(bool pressed) {
    if (pressed) {
        if (!host_seen) {
            show_no_link();
            return;
        }
        sent_encoder_press = true;
    } else {
        if (!sent_encoder_press) {
            return;
        }
        sent_encoder_press = false;
    }
    send_hid_key_event("ENC_SW", pressed ? 1U : 0U);
}

static uint32_t rainbow_color(uint8_t hue) {
    uint8_t segment = hue / 43U;
    uint8_t offset = (hue % 43U) * 6U;
    switch (segment) {
        case 0: return 0xFF0000UL | ((uint32_t)offset << 8);
        case 1: return ((uint32_t)(255U - offset) << 16) | 0x00FF00UL;
        case 2: return 0x00FF00UL | offset;
        case 3: return ((uint32_t)(255U - offset) << 8) | 0x0000FFUL;
        case 4: return ((uint32_t)offset << 16) | 0x0000FFUL;
        default: return 0xFF0000UL | (255U - offset);
    }
}

/* 根据相对色相/饱和度分类，不返回替代 RGB，也不代表宿主明确下发的业务状态。 */
static codex_micro_color_family_t classify_light_color(uint32_t color) {
    uint16_t red = (color >> 16) & 0xFFU;
    uint16_t green = (color >> 8) & 0xFFU;
    uint16_t blue = color & 0xFFU;
    uint16_t maximum = red > green ? red : green;
    uint16_t minimum = red < green ? red : green;
    if (blue > maximum) {
        maximum = blue;
    }
    if (blue < minimum) {
        minimum = blue;
    }
    if (maximum == 0) {
        return CODEX_MICRO_COLOR_DARK;
    }
    uint16_t span = maximum - minimum;
    if ((uint32_t)span * 100U <= (uint32_t)maximum * CODEX_MICRO_NEUTRAL_SATURATION_PERCENT) {
        return CODEX_MICRO_COLOR_NEUTRAL;
    }

    int32_t hue;
    if (maximum == red) {
        hue = 60 * ((int32_t)green - blue) / span;
    } else if (maximum == green) {
        hue = 120 + 60 * ((int32_t)blue - red) / span;
    } else {
        hue = 240 + 60 * ((int32_t)red - green) / span;
    }
    if (hue < 0) {
        hue += 360;
    }
    if (hue < 15 || hue >= 345) {
        return CODEX_MICRO_COLOR_RED;
    }
    if (hue < 75) {
        return CODEX_MICRO_COLOR_AMBER;
    }
    if (hue < 140) {
        return CODEX_MICRO_COLOR_GREEN;
    }
    if (hue < 175) {
        return CODEX_MICRO_COLOR_TEAL;
    }
    if (hue < 195) {
        return CODEX_MICRO_COLOR_CYAN;
    }
    if (hue < 275) {
        return CODEX_MICRO_COLOR_BLUE;
    }
    return CODEX_MICRO_COLOR_OTHER;
}

/* 仅调整本次输出的副本，宿主保存的颜色、亮度、效果和速度保持不变。 */
static void apply_light_adjustments(const codex_micro_light_t *light, bool task_light, uint16_t *brightness, codex_micro_effect_t *effect) {
    codex_micro_color_family_t family = classify_light_color(light->color);
    if (task_light) {
        if (family == CODEX_MICRO_COLOR_NEUTRAL && *effect == CODEX_MICRO_EFFECT_SOLID) {
            *brightness = (uint16_t)((uint32_t)*brightness * POC_CODEX_IDLE_BRIGHTNESS_PERCENT / 100U);
        } else if (*effect == CODEX_MICRO_EFFECT_SNAKE) {
            *effect = CODEX_MICRO_EFFECT_BREATH;
        }
        return;
    }
    if (*effect == CODEX_MICRO_EFFECT_SNAKE) {
        if (family == CODEX_MICRO_COLOR_BLUE) {
            *brightness = (uint16_t)((uint32_t)*brightness * POC_CODEX_RUNNING_AMBIENT_BRIGHTNESS_PERCENT / 100U);
        }
        *effect = family == CODEX_MICRO_COLOR_TEAL ? CODEX_MICRO_EFFECT_SOLID : CODEX_MICRO_EFFECT_BREATH;
    }
}

static uint32_t light_pixel(codex_micro_light_t *light, uint8_t index, uint8_t count, bool task_light) {
    if (!light->assigned || light->effect == CODEX_MICRO_EFFECT_OFF) {
        return 0;
    }

    advance_light_clock(light);
    uint16_t brightness = light->brightness;
    uint32_t color = light->color;
    codex_micro_effect_t effect = light->effect;
    apply_light_adjustments(light, task_light, &brightness, &effect);

    uint16_t phase = (uint16_t)(light->animation_clock / 16000U);
    uint16_t ramp = phase <= 512U ? phase : 1024U - phase;
    if (effect == CODEX_MICRO_EFFECT_BREATH) {
        brightness = (uint16_t)((uint32_t)brightness * ramp / 512U);
    } else if (effect == CODEX_MICRO_EFFECT_SHALLOW_BREATH) {
        brightness = (uint16_t)((uint32_t)brightness * (500U + (uint32_t)ramp * 500U / 512U) / CODEX_MICRO_LIGHT_SCALE);
    } else if (effect == CODEX_MICRO_EFFECT_RAINBOW || effect == CODEX_MICRO_EFFECT_GRADIENT) {
        color = rainbow_color((uint8_t)(light->animation_clock / 64000U + (uint16_t)index * 256U / count));
    }

    uint32_t red = (((color >> 16) & 0xFFU) * brightness + CODEX_MICRO_LIGHT_SCALE / 2U) / CODEX_MICRO_LIGHT_SCALE;
    uint32_t green = (((color >> 8) & 0xFFU) * brightness + CODEX_MICRO_LIGHT_SCALE / 2U) / CODEX_MICRO_LIGHT_SCALE;
    uint32_t blue = ((color & 0xFFU) * brightness + CODEX_MICRO_LIGHT_SCALE / 2U) / CODEX_MICRO_LIGHT_SCALE;
    return (red << 16) | (green << 8) | blue;
}

const char *codex_micro_task_status(uint8_t slot) {
    if (slot >= CODEX_MICRO_HOST_SLOT_COUNT || !thread_lights[slot].assigned || thread_lights[slot].effect == CODEX_MICRO_EFFECT_OFF) {
        return "--";
    }
    switch (classify_light_color(thread_lights[slot].color)) {
        case CODEX_MICRO_COLOR_DARK: return "OTHER";
        case CODEX_MICRO_COLOR_NEUTRAL: return "IDLE";
        case CODEX_MICRO_COLOR_BLUE: return "RUN";
        case CODEX_MICRO_COLOR_GREEN:
        case CODEX_MICRO_COLOR_TEAL: return "NEW";
        case CODEX_MICRO_COLOR_AMBER: return "WAIT";
        case CODEX_MICRO_COLOR_RED: return "ERR";
        default: return "OTHER";
    }
}

static bool has_extended_task_light(uint8_t led) {
    if (!host_seen) {
        return false;
    }
    for (uint8_t slot = CODEX_MICRO_TASK_KEY_COUNT; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        if (task_led_index[slot] == led && thread_lights[slot].assigned) {
            return true;
        }
    }
    return false;
}

bool codex_micro_rgb_indicators(uint8_t led_min, uint8_t led_max) {
    if (get_highest_layer(layer_state | default_layer_state) != POC_CODEX_LAYER) {
        return true;
    }

    for (uint8_t led = led_min; led < led_max && led < RGB_MATRIX_LED_COUNT; ++led) {
        RGB_MATRIX_INDICATOR_SET_COLOR(led, 0, 0, 0);
    }

    for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        uint8_t led = task_led_index[slot];
        if (led < led_min || led >= led_max) {
            continue;
        }

        uint32_t color = host_seen ? light_pixel(&thread_lights[slot], 0, 1, true) : 0;
        RGB_MATRIX_INDICATOR_SET_COLOR(led, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);
    }

    // ACT 键平时不亮，只有按住期间才点白光。
    for (uint8_t action = 0; action < CODEX_MICRO_ACTION_COUNT; ++action) {
        uint8_t led = act_led_index[action];
        if (led < led_min || led >= led_max || has_extended_task_light(led)) {
            continue;
        }
        if ((act_pressed_mask & (uint8_t)(1U << action)) == 0) {
            continue;
        }
        RGB_MATRIX_INDICATOR_SET_COLOR(led, 255, 255, 255);
    }

    if (host_seen && ambient_light.assigned) {
        for (uint8_t index = 0; index < sizeof(ambient_led_index) / sizeof(ambient_led_index[0]); ++index) {
            uint8_t led = ambient_led_index[index];
            if (led < led_min || led >= led_max || has_extended_task_light(led)) {
                continue;
            }
            uint32_t color = light_pixel(&ambient_light, index, sizeof(ambient_led_index) / sizeof(ambient_led_index[0]), false);
            RGB_MATRIX_INDICATOR_SET_COLOR(led, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color);
        }
    }

    return true;
}

void codex_micro_handle_report(uint8_t *data, uint8_t length) {
    uint8_t offset = length > 0 && data[0] == CODEX_MICRO_REPORT_ID ? 1 : 0;
    if (length < offset + 2 || data[offset] != CODEX_MICRO_CHANNEL_RPC) {
        return;
    }

    uint8_t payload_length = data[offset + 1];
    if (payload_length > CODEX_MICRO_FRAGMENT_SIZE || length < offset + 2 + payload_length) {
        return;
    }

    const char top_level_prefix[] = "{\"method\"";
    if (rpc_length > 0 && payload_length >= sizeof(top_level_prefix) - 1 && memcmp(&data[offset + 2], top_level_prefix, sizeof(top_level_prefix) - 1) == 0) {
        rpc_length = 0;
    }
    if (rpc_length + payload_length >= sizeof(rpc_buffer)) {
        rpc_length = 0;
        oled_controller_show_popup("CODEX MICRO", "RPC TOO LARGE", OLED_POPUP_ERROR_MS);
        return;
    }

    memcpy(rpc_buffer + rpc_length, &data[offset + 2], payload_length);
    rpc_length += payload_length;
    rpc_buffer[rpc_length] = '\0';
    process_rpc_buffer();
}
