#include QMK_KEYBOARD_H

#include "poc_config.h"
#include "poc_oled.h"
#include "codex_micro_protocol.h"

#ifdef POC_OLED_QUOTA
#    include "poc_quota.h"
#endif

#include <stdio.h>

#ifdef POC_OLED_STATUS

static char task_letter(uint8_t status) {
    switch (status) {
        case 1:
            return 'R';
        case 2:
            return 'W';
        case 3:
            return 'U';
        case 4:
            return 'E';
        default:
            return '.';
    }
}

static const char *layer_name(uint8_t layer) {
    switch (layer) {
        case 0:
            return "CODEX";
        case 1:
            return "NUM";
        case 2:
            return "NAV";
        case 3:
            return "SYS";
        default:
            return "?   ";
    }
}

void poc_oled_task(void) {
    char line[22];

    uint8_t layer = get_highest_layer(layer_state);
    oled_clear();

    // Line 1: layer number/name. No host program needed.
    oled_set_cursor(0, 0);
    snprintf(line, sizeof(line), "L%u %-5s", layer, layer_name(layer));
    oled_write(line, false);

    // Line 2: six task slots. Data comes from ChatGPT's v.oai.thstatus.
    oled_set_cursor(0, 1);
    snprintf(line, sizeof(line), "1%c 2%c 3%c 4%c 5%c 6%c", task_letter(codex_micro_slot_status(0)), task_letter(codex_micro_slot_status(1)), task_letter(codex_micro_slot_status(2)), task_letter(codex_micro_slot_status(3)), task_letter(codex_micro_slot_status(4)), task_letter(codex_micro_slot_status(5)));
    oled_write(line, false);

#ifdef POC_OLED_QUOTA
    uint8_t five = 0, weekly = 0;
    char    credits[8] = {0};
    if (poc_quota_get(&five, &weekly, credits, sizeof(credits))) {
        oled_set_cursor(0, 2);
        snprintf(line, sizeof(line), "5H%3u%% 7D%3u%%", five, weekly);
        oled_write(line, false);
        oled_set_cursor(0, 3);
        if (credits[0] != '\0') {
            snprintf(line, sizeof(line), "CR %s", credits);
        } else {
            snprintf(line, sizeof(line), "CR --");
        }
        oled_write(line, false);
        return;
    }
#endif

    // Default: no quota. No extra host program required.
    oled_set_cursor(0, 2);
    oled_write("R run W wait U new", false);
    oled_set_cursor(0, 3);
    oled_write("E err . idle", false);
}

#endif
