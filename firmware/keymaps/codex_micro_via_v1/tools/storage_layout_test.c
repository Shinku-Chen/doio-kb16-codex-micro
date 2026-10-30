#include <assert.h>
#include <stdio.h>

#include "test_qmk.h"
#include "poc_config.h"

#define DYNAMIC_KEYMAP_EEPROM_ADDR 128
#include "config.h"

int main(void) {
    assert(DYNAMIC_KEYMAP_LAYER_COUNT == 4);
    assert(POC_VIA_LAYER_COUNT == 4 && POC_NORMAL_LAYER_COUNT == 3 && POC_CODEX_LAYER == 3);
    assert(DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR == DYNAMIC_KEYMAP_EEPROM_ADDR + 5 * MATRIX_ROWS * MATRIX_COLS * 2);
    assert(DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR == DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR + 5 * NUM_ENCODERS * 4);
    assert(DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR - (DYNAMIC_KEYMAP_EEPROM_ADDR + DYNAMIC_KEYMAP_LAYER_COUNT * MATRIX_ROWS * MATRIX_COLS * 2) == 40);
    assert(DYNAMIC_KEYMAP_MACRO_EEPROM_ADDR - (DYNAMIC_KEYMAP_ENCODER_EEPROM_ADDR + DYNAMIC_KEYMAP_LAYER_COUNT * NUM_ENCODERS * 4) == 12);
    puts("PASS: four active layers retain legacy encoder/macro addresses without moving saved data");
    return 0;
}
