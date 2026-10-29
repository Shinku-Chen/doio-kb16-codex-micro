#pragma once

typedef enum {
    POC_USB_DESC_VIA = 0,
    POC_USB_DESC_CODEX = 1,
} poc_usb_desc_t;

poc_usb_desc_t poc_usb_desc(void);
void           poc_usb_select(poc_usb_desc_t desc);
