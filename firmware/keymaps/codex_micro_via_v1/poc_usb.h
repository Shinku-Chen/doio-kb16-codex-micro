#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POC_USB_DESC_VIA = 0,
    POC_USB_DESC_CODEX = 1,
} poc_usb_desc_t;

void           poc_usb_select(poc_usb_desc_t desc);
bool           poc_usb_is_codex(void);

// Consumed by firmware/qmk-core-patches.
bool     poc_usb_via_mode(void);
uint16_t poc_raw_hid_epsize(void);
bool     poc_usb_control_ready(void);
bool     poc_usb_control_push(const uint8_t *data, uint8_t length);
bool     poc_usb_control_pop(uint8_t *data, uint8_t *length);
