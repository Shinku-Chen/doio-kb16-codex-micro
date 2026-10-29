#include QMK_KEYBOARD_H
#include "poc_usb.h"

#ifdef POC_MODE_SWITCH
static poc_usb_desc_t current_desc = POC_USB_DESC_VIA;

poc_usb_desc_t poc_usb_desc(void) {
    return current_desc;
}

void poc_usb_select(poc_usb_desc_t desc) {
    current_desc = desc;
}
#endif
