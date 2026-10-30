#include QMK_KEYBOARD_H

#include <stdio.h>
#include <string.h>

#include "oled_icons.h"

static uint8_t previous_keys[32];
static uint8_t previous_mods;

void poc_oled_input_reset(void) {
    memset(previous_keys, 0, sizeof(previous_keys));
    previous_mods = 0;
}

static void append_token(char *label, size_t capacity, const char *token) {
    size_t used = strlen(label);
    if (used + 1 < capacity) {
        snprintf(label + used, capacity - used, "%s%s", used == 0 ? "" : "+", token);
    }
}

static void format_mods(char *label, size_t capacity, uint8_t mods) {
    static const char *const names[] = {"CTRL", "SHIFT", "ALT", "GUI"};
    for (uint8_t modifier = 0; modifier < 4; ++modifier) {
        if (mods & ((1U << modifier) | (1U << (modifier + 4U)))) {
            append_token(label, capacity, names[modifier]);
        }
    }
}

static void format_key(uint8_t key, char *name, size_t capacity) {
    if (key >= 0x04 && key <= 0x1D) {
        snprintf(name, capacity, "%c", 'A' + key - 0x04);
    } else if (key >= 0x1E && key <= 0x26) {
        snprintf(name, capacity, "%c", '1' + key - 0x1E);
    } else if (key == 0x27) {
        snprintf(name, capacity, "0");
    } else if (key >= 0x3A && key <= 0x45) {
        snprintf(name, capacity, "F%u", key - 0x3A + 1U);
    } else if (key >= 0x68 && key <= 0x73) {
        snprintf(name, capacity, "F%u", key - 0x68 + 13U);
    } else if (key >= 0x59 && key <= 0x61) {
        snprintf(name, capacity, "KP%u", key - 0x59 + 1U);
    } else {
        const char *label = NULL;
        switch (key) {
            case 0x28: label = "ENTER"; break;
            case 0x29: label = "ESC"; break;
            case 0x2A: label = "BACKSPACE"; break;
            case 0x2B: label = "TAB"; break;
            case 0x2C: label = "SPACE"; break;
            case 0x2D: label = "-"; break;
            case 0x2E: label = "="; break;
            case 0x2F: label = "["; break;
            case 0x30: label = "]"; break;
            case 0x31: label = "BACKSLASH"; break;
            case 0x33: label = ";"; break;
            case 0x34: label = "QUOTE"; break;
            case 0x35: label = "GRAVE"; break;
            case 0x36: label = ","; break;
            case 0x37: label = "."; break;
            case 0x38: label = "/"; break;
            case 0x39: label = "CAPS"; break;
            case 0x46: label = "PRINT"; break;
            case 0x47: label = "SCROLL LOCK"; break;
            case 0x48: label = "PAUSE"; break;
            case 0x49: label = "INSERT"; break;
            case 0x4A: label = "HOME"; break;
            case 0x4B: label = "PAGE UP"; break;
            case 0x4C: label = "DELETE"; break;
            case 0x4D: label = "END"; break;
            case 0x4E: label = "PAGE DOWN"; break;
            case 0x4F: label = "RIGHT"; break;
            case 0x50: label = "LEFT"; break;
            case 0x51: label = "DOWN"; break;
            case 0x52: label = "UP"; break;
            case 0x53: label = "NUM LOCK"; break;
            case 0x54: label = "KP/"; break;
            case 0x55: label = "KP*"; break;
            case 0x56: label = "KP-"; break;
            case 0x57: label = "KP+"; break;
            case 0x58: label = "KP ENTER"; break;
            case 0x62: label = "KP0"; break;
            case 0x63: label = "KP."; break;
            case 0x65: label = "MENU"; break;
            case 0x74: label = "EXECUTE"; break;
            case 0x75: label = "HELP"; break;
            case 0x78: label = "STOP"; break;
            case 0x79: label = "AGAIN"; break;
            case 0x7A: label = "UNDO"; break;
            case 0x7B: label = "CUT"; break;
            case 0x7C: label = "COPY"; break;
            case 0x7D: label = "PASTE"; break;
            case 0x7E: label = "FIND"; break;
            case 0x7F: label = "MUTE"; break;
            case 0x80: label = "VOL UP"; break;
            case 0x81: label = "VOL DOWN"; break;
        }
        if (label != NULL) {
            snprintf(name, capacity, "%s", label);
        } else {
            snprintf(name, capacity, "0x%02X", key);
        }
    }
}

void poc_oled_note_keys(uint8_t mods, const uint8_t *data, uint8_t length, bool bitmap) {
    if (data == NULL && length != 0) {
        return;
    }
    uint8_t keys[sizeof(previous_keys)] = {0};
    if (bitmap) {
        if (length != 0) {
            memcpy(keys, data, length < sizeof(keys) ? length : sizeof(keys));
        }
    } else {
        for (uint8_t index = 0; index < length; ++index) {
            uint8_t key = data[index];
            if (key != 0) {
                keys[key / 8U] |= 1U << (key % 8U);
            }
        }
    }
    bool active = mods != 0;
    bool pressed = (mods & (uint8_t)~previous_mods) != 0;
    for (uint8_t index = 0; index < sizeof(keys); ++index) {
        active |= keys[index] != 0;
        pressed |= (keys[index] & (uint8_t)~previous_keys[index]) != 0;
    }
    memcpy(previous_keys, keys, sizeof(keys));
    previous_mods = mods;
    if (!pressed) {
        oled_controller_note_input(OLED_INPUT_KEYBOARD, NULL, active);
        return;
    }
    char label[OLED_INPUT_LABEL_SIZE] = {0};
    format_mods(label, sizeof(label), mods);
    for (uint16_t key = 1; key <= 255; ++key) {
        if ((keys[key / 8U] & (1U << (key % 8U))) == 0) {
            continue;
        }
        char name[12];
        format_key(key, name, sizeof(name));
        append_token(label, sizeof(label), name);
    }
    oled_controller_note_input(OLED_INPUT_KEYBOARD, label, active);
}

void poc_oled_note_mouse(uint8_t buttons, int16_t horizontal, int16_t vertical, int16_t wheel_vertical, int16_t wheel_horizontal) {
    char label[OLED_INPUT_LABEL_SIZE] = {0};
    for (uint8_t button = 0; button < 8; ++button) {
        if (buttons & (1U << button)) {
            char name[8];
            snprintf(name, sizeof(name), "BTN%u", button + 1U);
            append_token(label, sizeof(label), name);
        }
    }
    if (horizontal != 0) append_token(label, sizeof(label), horizontal > 0 ? "MOVE RIGHT" : "MOVE LEFT");
    if (vertical != 0) append_token(label, sizeof(label), vertical > 0 ? "MOVE DOWN" : "MOVE UP");
    if (wheel_vertical != 0) append_token(label, sizeof(label), wheel_vertical > 0 ? "SCROLL UP" : "SCROLL DOWN");
    if (wheel_horizontal != 0) append_token(label, sizeof(label), wheel_horizontal > 0 ? "SCROLL RIGHT" : "SCROLL LEFT");
    oled_controller_note_input(OLED_INPUT_MOUSE, label, buttons != 0);
}

void poc_oled_note_extra(uint16_t usage, bool consumer) {
    oled_input_source_t source = consumer ? OLED_INPUT_EXTRA : OLED_INPUT_SYSTEM;
    if (usage == 0) {
        oled_controller_note_input(source, NULL, false);
        return;
    }
    const char *name = NULL;
    if (consumer) {
        switch (usage) {
            case 0x006F: name = "BRIGHTNESS UP"; break;
            case 0x0070: name = "BRIGHTNESS DOWN"; break;
            case 0x00B2: name = "RECORD"; break;
            case 0x00B3: name = "FAST FORWARD"; break;
            case 0x00B4: name = "REWIND"; break;
            case 0x00B5: name = "NEXT TRACK"; break;
            case 0x00B6: name = "PREV TRACK"; break;
            case 0x00B7: name = "MEDIA STOP"; break;
            case 0x00B8: name = "EJECT"; break;
            case 0x00CD: name = "PLAY/PAUSE"; break;
            case 0x00E2: name = "MUTE"; break;
            case 0x00E9: name = "VOL UP"; break;
            case 0x00EA: name = "VOL DOWN"; break;
        }
    } else {
        switch (usage) {
            case 0x0081: name = "POWER"; break;
            case 0x0082: name = "SLEEP"; break;
            case 0x0083: name = "WAKE"; break;
            case 0x008F: name = "RESTART"; break;
        }
    }
    char label[OLED_INPUT_LABEL_SIZE];
    if (name != NULL) {
        snprintf(label, sizeof(label), "%s", name);
    } else {
        snprintf(label, sizeof(label), "%s 0x%04X", consumer ? "MEDIA" : "SYSTEM", usage);
    }
    oled_controller_note_input(source, label, true);
}
