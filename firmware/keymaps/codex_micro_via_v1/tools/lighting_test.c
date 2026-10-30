#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "codex_micro_protocol.h"
#include "oled_icons.h"
#include "poc_config.h"
#include "poc_power.h"

layer_state_t layer_state;
layer_state_t default_layer_state = 1UL;

static uint32_t fake_timer;
static uint32_t input_idle;
static uint8_t colors[RGB_MATRIX_LED_COUNT][3];

uint8_t get_highest_layer(layer_state_t state) {
    for (int bit = 31; bit >= 0; --bit) {
        if (state & (1UL << bit)) {
            return (uint8_t)bit;
        }
    }
    return 0;
}

uint32_t timer_read32(void) {
    return fake_timer;
}

bool rgb_matrix_get_suspend_state(void) { return false; }
uint32_t last_input_activity_elapsed(void) { return input_idle; }

void test_set_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    assert(index < RGB_MATRIX_LED_COUNT);
    colors[index][0] = red;
    colors[index][1] = green;
    colors[index][2] = blue;
}

void oled_controller_show_popup(const char *first, const char *second, uint16_t duration) {
    (void)first;
    (void)second;
    (void)duration;
}

void raw_hid_send(uint8_t *data, uint8_t length) {
    assert(length == CODEX_MICRO_REPORT_SIZE);
    assert(data[0] == CODEX_MICRO_REPORT_ID);
    assert(data[1] == CODEX_MICRO_CHANNEL_RPC);
    assert(data[2] <= CODEX_MICRO_FRAGMENT_SIZE);
}

static void reset_lighting(void) {
    fake_timer = 0;
    input_idle = 0;
    layer_state = 1UL << POC_CODEX_LAYER;
    default_layer_state = 1UL;
    memset(colors, 0, sizeof(colors));
    codex_micro_init();
}

static void feed_json(const char *json) {
    size_t length = strlen(json);
    for (size_t offset = 0; offset < length;) {
        uint8_t report[CODEX_MICRO_REPORT_SIZE] = {CODEX_MICRO_REPORT_ID, CODEX_MICRO_CHANNEL_RPC};
        size_t fragment = length - offset;
        if (fragment > CODEX_MICRO_FRAGMENT_SIZE) {
            fragment = CODEX_MICRO_FRAGMENT_SIZE;
        }
        report[2] = (uint8_t)fragment;
        memcpy(report + 3, json + offset, fragment);
        codex_micro_handle_report(report, sizeof(report));
        offset += fragment;
    }
}

static void set_task_light(uint8_t slot, uint32_t color, const char *brightness, const char *effect, const char *speed) {
    char json[256];
    int length = snprintf(json, sizeof(json), "{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":%u,\"c\":%lu,\"b\":%s,\"e\":%s,\"s\":%s}],\"id\":710020}", slot, (unsigned long)color, brightness, effect, speed);
    assert(length > 0 && (size_t)length < sizeof(json));
    feed_json(json);
}

static void set_ambient_light(uint32_t color, const char *brightness, const char *effect, const char *speed) {
    char json[256];
    int length = snprintf(json, sizeof(json), "{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":%lu,\"b\":%s,\"e\":%s,\"s\":%s}}}", (unsigned long)color, brightness, effect, speed);
    assert(length > 0 && (size_t)length < sizeof(json));
    feed_json(json);
}

static void render(void) {
    codex_micro_rgb_indicators(0, RGB_MATRIX_LED_COUNT);
}

static uint32_t led_color(uint8_t index) {
    return ((uint32_t)colors[index][0] << 16) | ((uint32_t)colors[index][1] << 8) | colors[index][2];
}

static uint32_t scaled_color(uint32_t color, uint8_t percent) {
    uint32_t red = (((color >> 16) & 0xFFU) * percent + 50U) / 100U;
    uint32_t green = (((color >> 8) & 0xFFU) * percent + 50U) / 100U;
    uint32_t blue = ((color & 0xFFU) * percent + 50U) / 100U;
    return (red << 16) | (green << 8) | blue;
}

static void assert_ambient_row(uint32_t color) {
    for (uint8_t led = 12; led < 16; ++led) {
        assert(led_color(led) == color);
    }
}

static void test_native_colors(void) {
    static const uint32_t palette[] = {0x304FFE, 0x00FF4C, 0xFF6D00, 0xFF0033, 0x4C6FFF, 0xFFA000, 0x35C759, 0xFF453A, 0x123456};
    reset_lighting();
    for (size_t index = 0; index < sizeof(palette) / sizeof(palette[0]); ++index) {
        set_task_light(0, palette[index], "1", "1", "0");
        render();
        assert(led_color(1) == palette[index]);
    }
    set_task_light(0, 0x304FFE, "0.5", "1", "0");
    render();
    assert(led_color(1) == 0x18287F);
    puts("PASS: GPT colors and non-idle host brightness remain unchanged");
}

static void test_idle_brightness(void) {
    reset_lighting();
    set_task_light(0, 0xFFFFFF, "1", "1", "0");
    render();
    assert(led_color(1) == 0x0D0D0D);
    set_task_light(0, 0xFFFFFF, "0.5", "1", "0");
    render();
    assert(led_color(1) == 0x060606);
    set_task_light(0, 0xFFFFFF, "0", "1", "0");
    render();
    assert(led_color(1) == 0);
    set_task_light(0, 0xFFFFFF, "1", "4", "0.4");
    render();
    assert(led_color(1) == 0);
    fake_timer = 683;
    render();
    assert(led_color(1) == 0xFFFFFF);
    set_task_light(0, 0xFFFFFF, "0.5", "4", "0");
    render();
    assert(led_color(1) == 0x808080);
    set_task_light(0, 0xFFFFFF, "0", "4", "0");
    render();
    assert(led_color(1) == 0);
    puts("PASS: Idle solid uses 5%; breathing spans 0-100% and respects host brightness");
}

static void test_effect_formats_and_speed(void) {
    static const char *const effects[] = {"4", "\"breath\""};
    for (size_t index = 0; index < sizeof(effects) / sizeof(effects[0]); ++index) {
        reset_lighting();
        set_task_light(0, 0x304FFE, "1", effects[index], "0.4");
        render();
        assert(led_color(1) == 0);
        fake_timer = 683;
        render();
        assert(led_color(1) == 0x304FFE);
        feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"s\":0}]}");
        fake_timer += 1000;
        render();
        assert(led_color(1) == 0x304FFE);
    }
    reset_lighting();
    set_task_light(0, 0x304FFE, "1", "4", "0.4");
    fake_timer = 341;
    render();
    uint8_t slow_blue = colors[1][2];
    reset_lighting();
    set_task_light(0, 0x304FFE, "1", "4", "0.8");
    fake_timer = 341;
    render();
    assert(colors[1][2] > slow_blue);
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}");
    render();
    assert(led_color(1) == 0);
    puts("PASS: numeric/string effects, host animation speed and speed-zero pause");
}

static void test_snake_breath(void) {
    static const char *const effects[] = {"2", "\"snake\""};
    static const uint8_t agents[] = {1, 2, 4, 5, 6, 7};
    static const uint32_t palette[] = {0xFFFFFF, 0xFFF0E6, 0xE6F3FF, 0x404040, 0x304FFE, 0x208050, 0x123456, 0xE9293E, 0x9933CC};
    static const char *const brightness[] = {"1", "0.5", "0"};
    static const uint8_t percentages[] = {100, 50, 0};
    static const uint32_t times[] = {0, 342, 683, 1024, 1366};
    static const uint8_t ramps[] = {0, 50, 100, 50, 0};
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        for (size_t color = 0; color < sizeof(palette) / sizeof(palette[0]); ++color) {
            for (size_t level = 0; level < sizeof(brightness) / sizeof(brightness[0]); ++level) {
                reset_lighting();
                for (uint8_t slot = 0; slot < sizeof(agents) / sizeof(agents[0]); ++slot) {
                    set_task_light(slot, palette[color], brightness[level], effects[effect], "0.4");
                }
                for (size_t tick = 0; tick < sizeof(times) / sizeof(times[0]); ++tick) {
                    fake_timer = times[tick];
                    render();
                    uint32_t expected = scaled_color(palette[color], percentages[level] * ramps[tick] / 100U);
                    for (uint8_t slot = 0; slot < sizeof(agents) / sizeof(agents[0]); ++slot) {
                        assert(led_color(agents[slot]) == expected);
                    }
                    assert_ambient_row(0);
                }
            }
        }
    }
    puts("PASS: numeric/string AG snake uses original-color 0-100% breathing across all six slots, without Idle or ambient dimming");
}

static void test_snake_breath_speed(void) {
    static const char *const effects[] = {"2", "\"snake\""};
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        reset_lighting();
        set_task_light(0, 0x304FFE, "1", effects[effect], "0");
        render();
        assert(led_color(1) == 0);
        fake_timer = 1000;
        render();
        assert(led_color(1) == 0);
        reset_lighting();
        set_task_light(0, 0x304FFE, "1", effects[effect], "0.4");
        fake_timer = 341;
        render();
        uint32_t paused = led_color(1);
        assert(paused != 0 && paused != 0x304FFE);
        feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"s\":0}]}");
        fake_timer += 1000;
        render();
        assert(led_color(1) == paused);
        feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"s\":0.8}]}");
        fake_timer += 171;
        render();
        assert(led_color(1) == 0x304FFE);
        feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0}]}");
        render();
        assert(led_color(1) == 0);
        reset_lighting();
        set_task_light(0, 0x304FFE, "1", effects[effect], "0.8");
        fake_timer = 341;
        render();
        assert(led_color(1) > paused);
    }
    reset_lighting();
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":3166206,\"b\":1,\"e\":2}]}");
    fake_timer = 683;
    render();
    assert(led_color(1) == 0x304FFE);
    puts("PASS: AG snake follows host speed changes, zero-speed pause/resume, off and the existing default for omitted speed");
}

static void test_snake_rgb_passthrough(void) {
    reset_lighting();
    set_task_light(0, 0xFFFFFF, "0.63", "2", "0.4");
    fake_timer = 683;
    render();
    for (uint16_t red = 0; red <= 255; red += 17) {
        for (uint16_t green = 0; green <= 255; green += 17) {
            for (uint16_t blue = 0; blue <= 255; blue += 17) {
                uint32_t color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                set_task_light(0, color, "0.63", "\"snake\"", "0");
                render();
                assert(led_color(1) == scaled_color(color, 63));
            }
        }
    }
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"b\":0}]}");
    fake_timer += 1000;
    render();
    assert(led_color(1) == 0);
    puts("PASS: 4096 RGB samples retain host color/brightness on AG snake breathing, including neutral colors and paused peaks");
}

static void test_remaining_effects(void) {
    reset_lighting();
    set_task_light(0, 0x304FFE, "1", "6", "0.4");
    render();
    assert(led_color(1) == 0x18287F);
    fake_timer = 683;
    render();
    assert(led_color(1) == 0x304FFE);
    for (uint8_t effect = 3; effect <= 5; effect += 2) {
        char numeric[4];
        snprintf(numeric, sizeof(numeric), "%u", effect);
        reset_lighting();
        set_task_light(0, 0x123456, "1", numeric, "0.4");
        render();
        assert(led_color(1) == 0xFF0000);
        fake_timer = 341;
        render();
        assert(led_color(1) != 0xFF0000);
        assert(led_color(1) != 0);
    }
    puts("PASS: rainbow, gradient and shallow breathing keep their effect type");
}

static void test_ambient_mapping(void) {
    reset_lighting();
    set_task_light(0, 0x304FFE, "1", "1", "0");
    codex_micro_send_action_key(CODEX_MICRO_ACTION_MIC, true);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_11, true);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":3050327,\"b\":0.5,\"e\":2,\"s\":0.4},\"keys\":{\"c\":16711680,\"b\":1,\"e\":1}},\"id\":710021}");
    render();
    assert(led_color(1) == 0x304FFE);
    assert_ambient_row(0x17462C);
    assert(led_color(0) == 0 && led_color(3) == 0 && led_color(8) == 0);
    fake_timer = 100;
    render();
    assert_ambient_row(0x17462C);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":0}}}");
    render();
    assert(led_color(13) == 0xFFFFFF && led_color(14) == 0xFFFFFF);
    assert(led_color(12) == 0 && led_color(15) == 0);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_MIC, false);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_11, false);
    render();
    assert_ambient_row(0);
    reset_lighting();
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":16777215,\"b\":1,\"e\":2,\"s\":0.4}}}");
    render();
    assert_ambient_row(0);
    fake_timer = 342;
    render();
    assert_ambient_row(0x808080);
    fake_timer = 683;
    render();
    assert_ambient_row(0xFFFFFF);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":1}}}");
    fake_timer = 1366;
    render();
    assert_ambient_row(0xFFFFFF);
    puts("PASS: recording solid, processing breath and ready solid use the entire fourth row");
}

static void test_preview_and_partial_updates(void) {
    reset_lighting();
    feed_json("{\"method\":\"lights.preview\",\"params\":{\"underglow\":{\"color\":3050327,\"brightness\":1,\"effect\":\"snake\",\"speed\":0.4}}}");
    render();
    assert_ambient_row(0x2E8B57);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0.5},\"keys\":{\"c\":16711680,\"b\":0,\"e\":0}}}");
    render();
    assert_ambient_row(0x17462C);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"keys\":{\"c\":16777215,\"b\":0,\"e\":0}}}");
    render();
    assert_ambient_row(0x17462C);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":99}}}");
    fake_timer = 100;
    render();
    assert_ambient_row(0x17462C);
    puts("PASS: preview aliases, partial updates and nested-object boundaries");
}

static void test_recording_solid_override(void) {
    static const char *const effects[] = {"2", "\"snake\""};
    static const uint32_t times[] = {0, 342, 683, 1024, 1366};
    static const uint8_t percentages[] = {0, 50, 100, 50, 0};
    for (size_t index = 0; index < sizeof(effects) / sizeof(effects[0]); ++index) {
        reset_lighting();
        set_task_light(0, 0x2E8B57, "1", effects[index], "0.4");
        char json[256];
        snprintf(json, sizeof(json), "{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":3050327,\"b\":1,\"e\":%s}}}", effects[index]);
        feed_json(json);
        for (size_t tick = 0; tick < sizeof(times) / sizeof(times[0]); ++tick) {
            fake_timer = times[tick];
            render();
            assert_ambient_row(0x2E8B57);
            assert(led_color(1) == scaled_color(0x2E8B57, percentages[tick]));
        }
        feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0.5}}}");
        render();
        assert_ambient_row(0x17462C);
        feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0}}}");
        render();
        assert_ambient_row(0);
        feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":1,\"e\":0}}}");
        render();
        assert_ambient_row(0);
    }
    puts("PASS: ambient recording stays solid and respects brightness/off while same-color AG snake breathes");
}

static void test_act_feedback_and_layers(void) {
    reset_lighting();
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":0},{\"id\":1,\"e\":0},{\"id\":2,\"e\":0},{\"id\":3,\"e\":0},{\"id\":4,\"e\":0},{\"id\":5,\"e\":0}]}");
    codex_micro_send_action_key(CODEX_MICRO_ACTION_06, true);
    render();
    assert(led_color(8) == 0xFFFFFF);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_06, false);
    render();
    assert(led_color(8) == 0);
    memset(colors, 17, sizeof(colors));
    layer_state = 1UL << POC_NORMAL_FIRST_LAYER;
    render();
    for (uint8_t index = 0; index < RGB_MATRIX_LED_COUNT; ++index) {
        assert(led_color(index) == 0x111111);
    }
    puts("PASS: ACT press feedback survives empty tasks; VIA layers bypass Codex lighting");
}

static void test_ambient_full_row_isolation(void) {
    static const uint8_t agents[] = {1, 2, 4, 5, 6, 7};
    static const uint32_t palette[] = {0x304FFE, 0x00FF4C, 0xFF6D00, 0xFF0033, 0x123456, 0x654321};
    reset_lighting();
    for (uint8_t slot = 0; slot < 6; ++slot) {
        set_task_light(slot, palette[slot], "1", "1", "0");
    }
    codex_micro_send_action_key(CODEX_MICRO_ACTION_06, true);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_12, true);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":3166206,\"b\":1,\"e\":2,\"s\":0.4}}}");
    static const uint32_t times[] = {0, 342, 683, 1024, 1366};
    static const uint32_t expected[] = {0, 0x02040D, 0x050819, 0x02040D, 0};
    for (size_t tick = 0; tick < sizeof(times) / sizeof(times[0]); ++tick) {
        fake_timer = times[tick];
        render();
        assert_ambient_row(expected[tick]);
        for (uint8_t slot = 0; slot < 6; ++slot) {
            assert(led_color(agents[slot]) == palette[slot]);
        }
        assert(led_color(8) == 0xFFFFFF);
        assert(led_color(0) == 0 && led_color(3) == 0 && led_color(9) == 0 && led_color(10) == 0 && led_color(11) == 0);
    }
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0.5}}}");
    fake_timer = 2049;
    render();
    assert_ambient_row(0x02040D);
    memset(colors, 17, sizeof(colors));
    codex_micro_rgb_indicators(13, 15);
    assert(led_color(13) == 0x02040D && led_color(14) == 0x02040D);
    for (uint8_t led = 0; led < 16; ++led) {
        if (led != 13 && led != 14) {
            assert(led_color(led) == 0x111111);
        }
    }
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":0}}}");
    render();
    assert(led_color(12) == 0 && led_color(13) == 0 && led_color(14) == 0 && led_color(15) == 0xFFFFFF);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_12, false);
    render();
    assert_ambient_row(0);
    puts("PASS: all four ambient LEDs breathe together, preserve AG/ACT lights and restore ACT12 press feedback on clear");
}

static void test_ambient_breath_speed_and_brightness(void) {
    static const char *const effects[] = {"2", "\"snake\""};
    static const uint32_t palette[] = {0xFFFFFF, 0x304FFE};
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        for (size_t color = 0; color < sizeof(palette) / sizeof(palette[0]); ++color) {
            reset_lighting();
            char json[256];
            snprintf(json, sizeof(json), "{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":%lu,\"b\":1,\"e\":%s,\"s\":0.4}}}", (unsigned long)palette[color], effects[effect]);
            feed_json(json);
            render();
            assert_ambient_row(0);
            fake_timer = 342;
            render();
            uint32_t half = color == 0 ? 0x808080 : 0x02040D;
            assert_ambient_row(half);
            feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"s\":0}}}");
            fake_timer += 1000;
            render();
            assert_ambient_row(half);
            feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"s\":0.8}}}");
            fake_timer += 171;
            render();
            assert(led_color(12) > half);
            assert_ambient_row(led_color(12));
            feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0}}}");
            render();
            assert_ambient_row(0);
            feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":0}}}");
            render();
            assert_ambient_row(0);
            feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":1,\"e\":1}}}");
            render();
            assert_ambient_row(palette[color]);
        }
    }
    reset_lighting();
    feed_json("{\"method\":\"lights.preview\",\"params\":{\"underglow\":{\"color\":16777215,\"brightness\":0.5,\"effect\":\"snake\",\"speed\":0.4}}}");
    render();
    assert_ambient_row(0);
    fake_timer = 342;
    render();
    assert_ambient_row(0x404040);
    fake_timer = 683;
    render();
    assert_ambient_row(0x808080);
    puts("PASS: numeric/string ambient snake follows speed/pause and host brightness; running blue peaks at 10%, processing white at 100%");
}

static void test_running_ambient_brightness_limit(void) {
    static const char *const effects[] = {"2", "\"snake\""};
    static const char *const brightness[] = {"1", "0.5", "0"};
    static const uint32_t palette[] = {0x304FFE, 0x4C6FFF};
    static const uint32_t times[] = {0, 342, 683, 1024, 1366};
    static const uint32_t expected[][3][5] = {
        {{0, 0x02040D, 0x050819, 0x02040D, 0}, {0, 0x010206, 0x02040D, 0x010206, 0}, {0, 0, 0, 0, 0}},
        {{0, 0x04060D, 0x080B1A, 0x04060D, 0}, {0, 0x020306, 0x04060D, 0x020306, 0}, {0, 0, 0, 0, 0}},
    };
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        for (size_t color = 0; color < sizeof(palette) / sizeof(palette[0]); ++color) {
            for (size_t level = 0; level < sizeof(brightness) / sizeof(brightness[0]); ++level) {
                reset_lighting();
                set_task_light(0, palette[color], "1", "1", "0");
                set_ambient_light(palette[color], brightness[level], effects[effect], "0.4");
                for (size_t tick = 0; tick < sizeof(times) / sizeof(times[0]); ++tick) {
                    fake_timer = times[tick];
                    render();
                    assert_ambient_row(expected[color][level][tick]);
                    assert(led_color(1) == palette[color]);
                }
            }
        }
    }
    puts("PASS: GPT/Cindy running blue ambient snake breathes 0-10% times host brightness across the fourth row without dimming AG");
}

static void test_running_ambient_limit_scope(void) {
    static const uint32_t palette[] = {0xFFFFFF, 0x2E8B57, 0xFF0033, 0xFFA000, 0x35C759, 0xFF453A, 0xD0060C};
    for (size_t color = 0; color < sizeof(palette) / sizeof(palette[0]); ++color) {
        reset_lighting();
        set_ambient_light(palette[color], "1", "2", "0.4");
        render();
        assert_ambient_row(palette[color] == 0x2E8B57 ? 0x2E8B57 : 0);
        fake_timer = 683;
        render();
        assert_ambient_row(palette[color]);
    }
    static const char *const effects[] = {"1", "\"solid\"", "4", "\"breath\"", "6", "\"shallowBreath\""};
    static const uint32_t running_colors[] = {0x304FFE, 0x4C6FFF, 0x304FFD, 0x4C6FFE, 0x2F54EB, 0x123456};
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        for (size_t color = 0; color < sizeof(running_colors) / sizeof(running_colors[0]); ++color) {
            reset_lighting();
            set_ambient_light(running_colors[color], "1", effects[effect], "0.4");
            fake_timer = 683;
            render();
            assert_ambient_row(running_colors[color]);
        }
    }
    reset_lighting();
    set_ambient_light(0x304FFE, "1", "2", "0.4");
    fake_timer = 683;
    render();
    assert_ambient_row(0x050819);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":16777215}}}");
    render();
    assert_ambient_row(0xFFFFFF);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":3166206,\"e\":4}}}");
    render();
    assert_ambient_row(0x304FFE);
    feed_json("{\"method\":\"lights.preview\",\"params\":{\"underglow\":{\"color\":3166206,\"brightness\":0.5,\"effect\":\"snake\",\"speed\":0}}}");
    fake_timer += 1000;
    render();
    assert_ambient_row(0x02040D);
    puts("PASS: running-only dimming preserves recording, processing, other colors/effects, partial updates and preview speed-zero pause");
}

static void test_cindy_task_status_palette(void) {
    static const uint32_t palette[] = {0x4C6FFF, 0xFFA000, 0x35C759, 0xFF453A, 0xFFFFFF, 0xD0060C};
    static const char *const statuses[] = {"RUN", "WAIT", "NEW", "ERR", "IDLE", "ERR"};
    static const uint8_t agents[] = {1, 2, 4, 5, 6, 7};
    reset_lighting();
    for (uint8_t slot = 0; slot < 6; ++slot) {
        set_task_light(slot, palette[slot], "1", "1", "0");
        assert(strcmp(codex_micro_task_status(slot), statuses[slot]) == 0);
    }
    render();
    for (uint8_t slot = 0; slot < 6; ++slot) {
        assert(led_color(agents[slot]) == (slot == 4 ? 0x0D0D0D : palette[slot]));
    }
    set_task_light(0, 0x4C6FFF, "0", "4", "0");
    render();
    assert(led_color(1) == 0 && strcmp(codex_micro_task_status(0), "RUN") == 0);
    set_task_light(0, 0x4C6FFF, "1", "0", "0");
    assert(strcmp(codex_micro_task_status(0), "--") == 0);
    set_task_light(0, 0x4C6FFE, "1", "1", "0");
    assert(strcmp(codex_micro_task_status(0), "RUN") == 0);
    puts("PASS: host palette fixtures use generic color estimates without recoloring LEDs or changing off/zero-brightness handling");
}

static void test_cindy_running_ambient_frame(void) {
    static const char *const effects[] = {"2", "\"snake\""};
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        reset_lighting();
        set_task_light(0, 0x4C6FFF, "0.8", "4", "0.35");
        set_ambient_light(0x4C6FFF, "0.7", effects[effect], "0.4");
        fake_timer = 683;
        render();
        assert_ambient_row(0x050812);
        assert(led_color(1) == 0x354EB3);
        assert(strcmp(codex_micro_task_status(0), "RUN") == 0);
        feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":16777215}}}");
        render();
        assert_ambient_row(0xB3B3B3);
        feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":5009407}}}");
        render();
        assert_ambient_row(0x050812);
        feed_json("{\"method\":\"lights.preview\",\"params\":{\"underglow\":{\"color\":5009407,\"brightness\":0.5,\"effect\":\"snake\",\"speed\":0}}}");
        fake_timer += 1000;
        render();
        assert_ambient_row(0x04060D);
        feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":0}}}");
        render();
        assert_ambient_row(0);
    }
    puts("PASS: installed Cindy running frame peaks at 7% with host B=0.7, preserves AG and supports partial updates/preview/pause/off");
}

static void test_generic_color_families(void) {
    static const uint32_t palette[] = {0x2F54EB, 0x304FFD, 0x4C6FFE, 0x123456, 0x31B54F, 0x208050, 0xE8A61C, 0xFFEA00, 0xE9293E, 0xD0060C, 0xFFF0E6, 0xE6F3FF, 0x404040, 0x00CCCC, 0x9933CC, 0};
    static const char *const statuses[] = {"RUN", "RUN", "RUN", "RUN", "NEW", "NEW", "WAIT", "WAIT", "ERR", "ERR", "IDLE", "IDLE", "IDLE", "OTHER", "OTHER", "OTHER"};
    static const char *const brightness[] = {"1", "0.4"};
    for (size_t level = 0; level < sizeof(brightness) / sizeof(brightness[0]); ++level) {
        reset_lighting();
        for (size_t color = 0; color < sizeof(palette) / sizeof(palette[0]); ++color) {
            set_task_light(0, palette[color], brightness[level], "1", "0");
            render();
            assert(strcmp(codex_micro_task_status(0), statuses[color]) == 0);
            uint8_t percent = level == 0 ? 100 : 40;
            if (strcmp(statuses[color], "IDLE") == 0) {
                percent /= 20;
            }
            assert(led_color(1) == scaled_color(palette[color], percent));
        }
    }
    reset_lighting();
    set_task_light(0, 0, "1", "3", "0.4");
    render();
    assert(led_color(1) == 0xFF0000);
    assert(strcmp(codex_micro_task_status(0), "OTHER") == 0);
    set_task_light(0, 0, "1", "0", "0");
    assert(strcmp(codex_micro_task_status(0), "--") == 0);
    puts("PASS: arbitrary hue families are classified without an App RGB whitelist; a black base color does not imply an unassigned/off task");
}

static void test_generic_rgb_passthrough(void) {
    reset_lighting();
    set_task_light(0, 0xFFFFFF, "0.63", "4", "0.4");
    set_ambient_light(0xFFFFFF, "0.63", "4", "0.4");
    fake_timer = 683;
    render();
    for (uint16_t red = 0; red <= 255; red += 17) {
        for (uint16_t green = 0; green <= 255; green += 17) {
            for (uint16_t blue = 0; blue <= 255; blue += 17) {
                uint32_t color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
                set_task_light(0, color, "0.63", "\"breath\"", "0");
                set_ambient_light(color, "0.63", "\"breath\"", "0");
                render();
                assert(led_color(1) == scaled_color(color, 63));
                assert_ambient_row(scaled_color(color, 63));
            }
        }
    }
    puts("PASS: 4096 RGB samples retain the host hue/brightness on AG and ambient native breathing, with speed-zero phase preserved");
}

static void test_adjustment_parameter_isolation(void) {
    reset_lighting();
    set_task_light(0, 0xFFF0E6, "0.8", "1", "0.4");
    set_ambient_light(0x2759CC, "0.65", "2", "0.4");
    fake_timer = 683;
    render();
    assert(led_color(1) == 0x0A0A09);
    assert_ambient_row(0x03060D);
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":4,\"s\":0}]}");
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":4,\"s\":0}}}");
    fake_timer += 1000;
    render();
    assert(led_color(1) == 0xCCC0B8);
    assert_ambient_row(0x193A85);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0.5,\"e\":2}}}");
    render();
    assert_ambient_row(0x02040A);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"e\":1}}}");
    render();
    assert_ambient_row(0x142D66);
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"c\":16773350,\"e\":2}}}");
    render();
    assert_ambient_row(0x807873);
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"e\":2}]}");
    render();
    assert(led_color(1) == 0xCCC0B8);
    fake_timer = 1700;
    render();
    assert(led_color(1) == 0xCCC0B8);
    assert_ambient_row(0x807873);
    puts("PASS: local Idle/snake adjustments do not mutate saved host RGB, brightness, effect or speed; AG snake preserves the paused phase");
}

static void test_generic_teal_recording(void) {
    static const uint32_t palette[] = {0x208050, 0x40B878, 0x307D65, 0x2E8B57};
    static const uint32_t times[] = {0, 342, 683, 1024, 1366};
    static const uint8_t percentages[] = {0, 50, 100, 50, 0};
    for (size_t color = 0; color < sizeof(palette) / sizeof(palette[0]); ++color) {
        reset_lighting();
        set_task_light(0, palette[color], "1", "2", "0.4");
        set_ambient_light(palette[color], "0.4", "\"snake\"", "0.4");
        for (size_t tick = 0; tick < sizeof(times) / sizeof(times[0]); ++tick) {
            fake_timer = times[tick];
            render();
            assert_ambient_row(scaled_color(palette[color], 40));
            assert(led_color(1) == scaled_color(palette[color], percentages[tick]));
        }
        set_ambient_light(palette[color], "1", "4", "0.4");
        fake_timer += 683;
        render();
        assert_ambient_row(palette[color]);
    }
    puts("PASS: teal shades preserve host RGB and recording-style solid override only for ambient snake, not AG or native breath");
}

static void test_extended_task_slots(void) {
    static const uint8_t leds[] = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15};
    static const char *const effects[] = {"2", "\"snake\""};
    for (size_t effect = 0; effect < sizeof(effects) / sizeof(effects[0]); ++effect) {
        reset_lighting();
        for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
            set_task_light(slot, 0x2759CC + slot, "0.5", effects[effect], "0.4");
        }
        fake_timer = 683;
        render();
        for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
            assert(led_color(leds[slot]) == scaled_color(0x2759CC + slot, 50));
            assert(strcmp(codex_micro_task_status(slot), "RUN") == 0);
        }
        assert(led_color(0) == 0 && led_color(3) == 0 && led_color(12) == 0);
        feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":12,\"s\":0},{\"id\":13,\"c\":16711680,\"b\":1,\"e\":1},{\"id\":255,\"e\":1}]}");
        fake_timer += 1000;
        render();
        assert(led_color(15) == scaled_color(0x2759CC + 12, 50));
        assert(strcmp(codex_micro_task_status(13), "--") == 0);
        assert(strcmp(codex_micro_task_status(255), "--") == 0);
    }
    puts("PASS: all thirteen task-light slots map to native AG and ACT positions, preserving RGB, brightness, snake breathing, pause and bounds");
}

static void test_extended_task_priority(void) {
    static const uint8_t leds[] = {8, 9, 10, 11, 13, 14, 15};
    reset_lighting();
    set_ambient_light(0x208050, "1", "2", "0.4");
    for (uint8_t action = 0; action < CODEX_MICRO_ACTION_COUNT; ++action) {
        codex_micro_send_action_key((codex_micro_action_t)action, true);
    }
    for (uint8_t slot = CODEX_MICRO_TASK_KEY_COUNT; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        set_task_light(slot, 0xFF0033, "1", "1", "0");
    }
    render();
    for (size_t index = 0; index < sizeof(leds) / sizeof(leds[0]); ++index) {
        assert(led_color(leds[index]) == 0xFF0033);
    }
    assert(led_color(12) == 0x208050);
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":6,\"b\":0},{\"id\":10,\"b\":0}]}");
    render();
    assert(led_color(8) == 0 && led_color(13) == 0);
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":6,\"e\":0},{\"id\":10,\"e\":0}]}");
    render();
    assert(led_color(8) == 0xFFFFFF && led_color(13) == 0x208050);
    set_ambient_light(0x208050, "1", "0", "0");
    render();
    assert(led_color(13) == 0xFFFFFF && led_color(14) == 0xFF0033);
    codex_micro_send_action_key(CODEX_MICRO_ACTION_06, false);
    render();
    assert(led_color(8) == 0);
    puts("PASS: assigned extended tasks, including brightness zero, override ACT white and ambient; off restores the previous feedback priority");
}

static void test_extended_fragmented_frame(void) {
    static const uint8_t leds[] = {1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15};
    char json[2048];
    size_t used = (size_t)snprintf(json, sizeof(json), "{\"method\":\"v.oai.thstatus\",\"params\":[");
    reset_lighting();
    for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        int length = snprintf(json + used, sizeof(json) - used, "%s{\"id\":%u,\"c\":%lu,\"b\":0.7000000000000001,\"e\":1,\"s\":0.35000000000000003,\"sk\":0,\"sa\":0}", slot == 0 ? "" : ",", slot, (unsigned long)(0x2759CC + slot));
        assert(length > 0 && (size_t)length < sizeof(json) - used);
        used += (size_t)length;
    }
    int length = snprintf(json + used, sizeof(json) - used, "],\"id\":710025}");
    assert(length > 0 && (size_t)length < sizeof(json) - used);
    used += (size_t)length;
    assert(used > 1024 && used < sizeof(json));
    feed_json(json);
    render();
    for (uint8_t slot = 0; slot < CODEX_MICRO_HOST_SLOT_COUNT; ++slot) {
        assert(led_color(leds[slot]) == scaled_color(0x2759CC + slot, 70));
    }
    puts("PASS: a fragmented thirteen-slot SDK-format frame larger than 1024 bytes is applied in full without truncating its final task");
}

static void test_off_partial_updates(void) {
    static const uint8_t slots[] = {0, 6, 12};
    static const uint8_t leds[] = {1, 8, 15};
    reset_lighting();
    for (size_t index = 0; index < sizeof(slots) / sizeof(slots[0]); ++index) {
        set_task_light(slots[index], 0xFF0033, "1", "0", "0");
    }
    set_ambient_light(0xFF0033, "1", "0", "0");
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0,\"c\":5009407},{\"id\":6,\"b\":0.5},{\"id\":12,\"s\":0.4}]}");
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{\"b\":0.5}}}");
    render();
    for (size_t index = 0; index < sizeof(leds) / sizeof(leds[0]); ++index) {
        assert(led_color(leds[index]) == 0);
        assert(strcmp(codex_micro_task_status(slots[index]), "--") == 0);
    }
    assert_ambient_row(0);
    feed_json("{\"method\":\"v.oai.thstatus\",\"params\":[{\"id\":0},{\"id\":6},{\"id\":12}]}");
    feed_json("{\"method\":\"v.oai.rgbcfg\",\"params\":{\"ambient\":{}}}");
    render();
    assert(led_color(1) == 0 && led_color(8) == 0 && led_color(15) == 0);
    set_task_light(6, 0xFF0033, "0.5", "1", "0");
    render();
    assert(led_color(8) == scaled_color(0xFF0033, 50));
    puts("PASS: omitted effects and empty partial updates preserve off for native/extended tasks and ambient, rather than silently relighting");
}

static void test_render_range_and_clock_wrap(void) {
    reset_lighting();
    set_task_light(0, 0xFF0033, "1", "1", "0");
    memset(colors, 17, sizeof(colors));
    codex_micro_rgb_indicators(4, 8);
    assert(led_color(1) == 0x111111 && led_color(8) == 0x111111);
    assert(led_color(4) == 0 && led_color(7) == 0);
    fake_timer = UINT32_MAX - 20;
    codex_micro_init();
    set_task_light(0, 0x304FFE, "1", "4", "0.4");
    fake_timer = 20;
    render();
    assert(colors[1][2] > 0 && colors[1][2] <= 254);
    puts("PASS: RGB render ranges and the 32-bit clock wrap are bounded");
}

static void test_host_power_packets(void) {
    reset_lighting();
    fake_timer = input_idle = POC_IDLE_SLEEP_MS;
    assert(poc_power_should_sleep());
    feed_json("{\"method\":\"device.status\",\"id\":710030}");
    assert(!poc_power_should_sleep());
    for (uint8_t hour = 0; hour < 3; ++hour) {
        fake_timer += POC_IDLE_SLEEP_MS - 1;
        input_idle += POC_IDLE_SLEEP_MS - 1;
        feed_json("{\"method\":\"device.status\",\"id\":710030}");
        assert(!poc_power_should_sleep());
    }
    fake_timer += POC_IDLE_SLEEP_MS;
    input_idle += POC_IDLE_SLEEP_MS;
    assert(poc_power_should_sleep());
    feed_json("{\"id\":710030,\"params\":{}}");
    assert(poc_power_should_sleep());
    set_task_light(0, 0x304FFE, "1", "1", "0");
    assert(!poc_power_should_sleep());
    fake_timer += POC_IDLE_SLEEP_MS;
    input_idle += POC_IDLE_SLEEP_MS;
    assert(poc_power_should_sleep());
    set_task_light(0, 0x304FFE, "1", "1", "0");
    assert(!poc_power_should_sleep());
    render();
    assert(led_color(1) == 0x304FFE);
    puts("PASS: real repeated status/lighting RPCs renew and wake the shared timer, while methodless malformed traffic does not");
}

static void test_host_power_light_states(void) {
    reset_lighting();
    set_task_light(0, 0x304FFE, "1", "4", "0");
    render();
    assert(led_color(1) == 0 && !poc_power_should_sleep());
    set_ambient_light(0x208050, "1", "1", "0");
    set_task_light(0, 0x304FFE, "0", "4", "0");
    assert(!poc_power_should_sleep());
    set_ambient_light(0x208050, "0", "1", "0");
    assert(poc_power_should_sleep());
    feed_json("{\"method\":\"device.status\",\"id\":710031}");
    assert(poc_power_should_sleep());
    set_task_light(12, 0, "1", "3", "0.4");
    assert(!poc_power_should_sleep());
    set_task_light(12, 0, "1", "0", "0");
    assert(poc_power_should_sleep());
    set_task_light(6, 0xFF0033, "1", "1", "0");
    assert(!poc_power_should_sleep());
    set_task_light(6, 0, "1", "1", "0");
    assert(poc_power_should_sleep());
    puts("PASS: power follows aggregate native/extended/ambient host states, not breath minima; all-off/zero stays asleep across heartbeat and black-base rainbow remains active");
}

int main(void) {
    test_native_colors();
    test_idle_brightness();
    test_effect_formats_and_speed();
    test_snake_breath();
    test_snake_breath_speed();
    test_snake_rgb_passthrough();
    test_remaining_effects();
    test_ambient_mapping();
    test_preview_and_partial_updates();
    test_recording_solid_override();
    test_act_feedback_and_layers();
    test_ambient_full_row_isolation();
    test_ambient_breath_speed_and_brightness();
    test_running_ambient_brightness_limit();
    test_running_ambient_limit_scope();
    test_cindy_task_status_palette();
    test_cindy_running_ambient_frame();
    test_generic_color_families();
    test_generic_rgb_passthrough();
    test_adjustment_parameter_isolation();
    test_generic_teal_recording();
    test_extended_task_slots();
    test_extended_task_priority();
    test_extended_fragmented_frame();
    test_off_partial_updates();
    test_render_range_and_clock_wrap();
    test_host_power_packets();
    test_host_power_light_states();
    return 0;
}
