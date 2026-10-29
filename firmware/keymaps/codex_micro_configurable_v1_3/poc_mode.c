#include QMK_KEYBOARD_H

#include "poc_config.h"
#include "poc_mode.h"
#include "poc_usb.h"

#ifdef POC_MODE_SWITCH

__attribute__((section(".noinit"))) static uint32_t poc_boot_magic;

static poc_mode_t current_mode = POC_MODE_VIA;
static uint32_t   via_started_at;
static bool       via_seen;
static bool       via_timer_started;

static void poc_set_magic(uint32_t value) {
    poc_boot_magic = value;
}

void poc_mode_init(void) {
    if (poc_boot_magic == POC_MODE_MAGIC) {
        current_mode   = POC_MODE_CODEX;
        poc_boot_magic = 0;
        poc_usb_select(POC_USB_DESC_CODEX);
        return;
    }

    current_mode      = POC_MODE_VIA;
    via_started_at    = 0;
    via_seen          = false;
    via_timer_started = false;
    poc_usb_select(POC_USB_DESC_VIA);
}

void poc_mode_note_via_command(void) {
    if (current_mode != POC_MODE_VIA) {
        return;
    }
    if (!via_timer_started) {
        via_started_at    = timer_read32();
        via_timer_started = true;
    }
    via_seen = true;
}

void poc_mode_force_codex(void) {
    if (current_mode == POC_MODE_CODEX) {
        return;
    }
    poc_set_magic(POC_MODE_MAGIC);
    poc_usb_select(POC_USB_DESC_CODEX);
    reset_keyboard();
}

void poc_mode_force_via(void) {
    if (current_mode == POC_MODE_VIA) {
        return;
    }
    poc_set_magic(0);
    poc_usb_select(POC_USB_DESC_VIA);
    reset_keyboard();
}

void poc_mode_task(void) {
    if (current_mode != POC_MODE_VIA) {
        return;
    }

    if (!via_timer_started) {
        via_started_at    = timer_read32();
        via_timer_started = true;
    }

    // Once VIA has connected, stay in VIA until the keyboard is power-cycled.
    if (!via_seen && timer_elapsed32(via_started_at) >= POC_VIA_WINDOW_MS) {
        poc_mode_force_codex();
    }
}

poc_mode_t poc_mode_get(void) {
    return current_mode;
}

bool poc_mode_is_via(void) {
    return current_mode == POC_MODE_VIA;
}

#endif
