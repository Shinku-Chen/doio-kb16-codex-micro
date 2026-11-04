#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_qmk.h"
#include "config.h"
#include "poc_config.h"

#define VIA_EEPROM_LAYOUT_OPTIONS_DEFAULT 0

static uint8_t  via_magic[3];
static unsigned magic_writes;
static unsigned erase_count;
static unsigned keymap_resets;
static unsigned macro_resets;
static unsigned layout_writes;
static uint32_t layout_value;
static uint8_t  keymap_data[4][4][5];
static uint8_t  encoder_data[4][3][2];
static uint8_t  macro_data[16];

void nvm_via_erase(void) {
    ++erase_count;
}

void nvm_via_read_magic(uint8_t *magic0, uint8_t *magic1, uint8_t *magic2) {
    *magic0 = via_magic[0];
    *magic1 = via_magic[1];
    *magic2 = via_magic[2];
}

void nvm_via_update_magic(uint8_t magic0, uint8_t magic1, uint8_t magic2) {
    via_magic[0] = magic0;
    via_magic[1] = magic1;
    via_magic[2] = magic2;
    ++magic_writes;
}

uint32_t nvm_via_read_layout_options(void) {
    return 0x11223344;
}

void nvm_via_update_layout_options(uint32_t value) {
    layout_value = value;
    ++layout_writes;
}

void dynamic_keymap_reset(void) {
    ++keymap_resets;
    memset(keymap_data, 0x42, sizeof(keymap_data));
    memset(encoder_data, 0x24, sizeof(encoder_data));
}

void dynamic_keymap_macro_reset(void) {
    ++macro_resets;
    memset(macro_data, 0, sizeof(macro_data));
}

void     eeconfig_init_via(void);
void     via_init(void);
uint32_t via_get_layout_options(void);
void     via_set_layout_options(uint32_t value);

void via_init_kb(void) {}
void via_set_layout_options_kb(uint32_t value) {
    (void)value;
}

#include "via_init_under_test.h"

static void reset_counters(void) {
    magic_writes  = 0;
    erase_count   = 0;
    keymap_resets = 0;
    macro_resets  = 0;
    layout_writes = 0;
    layout_value  = 0;
}

static void fill_sentinels(void) {
    memset(keymap_data, 0xA5, sizeof(keymap_data));
    memset(encoder_data, 0x5A, sizeof(encoder_data));
    memset(macro_data, 0xC3, sizeof(macro_data));
}

static void assert_sentinels(void) {
    for (size_t index = 0; index < sizeof(keymap_data); ++index) assert(((uint8_t *)keymap_data)[index] == 0xA5);
    for (size_t index = 0; index < sizeof(encoder_data); ++index) assert(((uint8_t *)encoder_data)[index] == 0x5A);
    for (size_t index = 0; index < sizeof(macro_data); ++index) assert(macro_data[index] == 0xC3);
}

static void assert_defaults(void) {
    for (size_t index = 0; index < sizeof(keymap_data); ++index) assert(((uint8_t *)keymap_data)[index] == 0x42);
    for (size_t index = 0; index < sizeof(encoder_data); ++index) assert(((uint8_t *)encoder_data)[index] == 0x24);
    for (size_t index = 0; index < sizeof(macro_data); ++index) assert(macro_data[index] == 0);
}

static void set_magic(uint8_t magic0, uint8_t magic1, uint8_t magic2) {
    via_magic[0] = magic0;
    via_magic[1] = magic1;
    via_magic[2] = magic2;
}

static void assert_current_marker(void) {
    assert(via_magic[0] == VIA_EEPROM_MARKER_0);
    assert(via_magic[1] == VIA_EEPROM_MARKER_1);
    assert(via_magic[2] == VIA_EEPROM_SCHEMA_VERSION);
}

static void test_current_marker_preserves(void) {
    fill_sentinels();
    reset_counters();
    set_magic(VIA_EEPROM_MARKER_0, VIA_EEPROM_MARKER_1, VIA_EEPROM_SCHEMA_VERSION);

    via_init();

    assert(magic_writes == 0 && erase_count == 0 && keymap_resets == 0 && macro_resets == 0 && layout_writes == 0);
    assert_current_marker();
    assert_sentinels();
    puts("PASS: current VIA marker preserves custom keymap, three encoders and macros with zero EEPROM writes");
}

static void test_invalid_marker_resets_once(const char *name, uint8_t magic0, uint8_t magic1, uint8_t magic2) {
    fill_sentinels();
    reset_counters();
    set_magic(magic0, magic1, magic2);

    via_init();

    assert(erase_count == 1 && keymap_resets == 1 && macro_resets == 1 && layout_writes == 1);
    assert(layout_value == VIA_EEPROM_LAYOUT_OPTIONS_DEFAULT);
    assert_current_marker();
    assert_defaults();

    unsigned saved_magic_writes  = magic_writes;
    unsigned saved_erase_count   = erase_count;
    unsigned saved_keymap_resets = keymap_resets;
    unsigned saved_macro_resets  = macro_resets;
    unsigned saved_layout_writes = layout_writes;

    via_init();

    assert(magic_writes == saved_magic_writes && erase_count == saved_erase_count);
    assert(keymap_resets == saved_keymap_resets && macro_resets == saved_macro_resets);
    assert(layout_writes == saved_layout_writes);
    assert_current_marker();
    assert_defaults();
    printf("PASS: %s marker initializes VIA defaults once, writes the current marker, and stays stable on restart\n", name);
}

static void test_default_init_helper(void) {
    fill_sentinels();
    reset_counters();
    set_magic(VIA_EEPROM_MARKER_0, VIA_EEPROM_MARKER_1, VIA_EEPROM_SCHEMA_VERSION);

    eeconfig_init_via();

    assert(erase_count == 1 && keymap_resets == 1 && macro_resets == 1 && layout_writes == 1);
    assert_current_marker();
    assert_defaults();
    puts("PASS: default init helper rebuilds defaults and writes the current marker");
}

int main(void) {
    assert(EECONFIG_USER_DATA_SIZE == 8);
    assert(POC_EEPROM_RESERVED_LAYER_SLOTS == 5);
    puts("PASS: user EEPROM size and reserved keymap/encoder/macro capacity remain unchanged");

    test_current_marker_preserves();
    test_invalid_marker_resets_once("old build-date", 0x26, 0x11, 0x04);
    test_invalid_marker_resets_once("old schema", VIA_EEPROM_MARKER_0, VIA_EEPROM_MARKER_1, 0x00);
    test_invalid_marker_resets_once("unknown", 0x12, 0x34, 0x56);
    test_invalid_marker_resets_once("all-FF", 0xFF, 0xFF, 0xFF);
    test_invalid_marker_resets_once("all-00", 0x00, 0x00, 0x00);
    test_default_init_helper();
    return 0;
}
