#include QMK_KEYBOARD_H

#include <stdatomic.h>
#include <string.h>

#include "poc_usb.h"

#define POC_CONTROL_QUEUE_SIZE 16

typedef struct {
    uint8_t data[64];
    uint8_t length;
} poc_control_report_t;

static poc_usb_desc_t current_desc = POC_USB_DESC_VIA;
static poc_control_report_t control_reports[POC_CONTROL_QUEUE_SIZE];
static atomic_uchar control_head;
static atomic_uchar control_tail;

void poc_usb_select(poc_usb_desc_t desc) {
    current_desc = desc;
    atomic_store(&control_head, 0);
    atomic_store(&control_tail, 0);
}

bool poc_usb_is_codex(void) {
    return current_desc == POC_USB_DESC_CODEX;
}

// The two hooks below are referenced by the patched QMK USB stack. They are
// safe to call from the USB setup interrupt: the value only changes at boot.
bool poc_usb_via_mode(void) {
    return !poc_usb_is_codex();
}

uint16_t poc_raw_hid_epsize(void) {
    return poc_usb_is_codex() ? 64 : 32;
}

bool poc_usb_control_ready(void) {
    uint8_t next = (atomic_load_explicit(&control_head, memory_order_relaxed) + 1) % POC_CONTROL_QUEUE_SIZE;
    return next != atomic_load_explicit(&control_tail, memory_order_acquire);
}

bool poc_usb_control_push(const uint8_t *data, uint8_t length) {
    if (length == 0 || length > sizeof(control_reports[0].data) || !poc_usb_control_ready()) {
        return false;
    }
    uint8_t head = atomic_load_explicit(&control_head, memory_order_relaxed);
    memcpy(control_reports[head].data, data, length);
    control_reports[head].length = length;
    atomic_store_explicit(&control_head, (head + 1) % POC_CONTROL_QUEUE_SIZE, memory_order_release);
    return true;
}

bool poc_usb_control_pop(uint8_t *data, uint8_t *length) {
    uint8_t tail = atomic_load_explicit(&control_tail, memory_order_relaxed);
    if (tail == atomic_load_explicit(&control_head, memory_order_acquire)) {
        return false;
    }
    *length = control_reports[tail].length;
    memcpy(data, control_reports[tail].data, *length);
    atomic_store_explicit(&control_tail, (tail + 1) % POC_CONTROL_QUEUE_SIZE, memory_order_release);
    return true;
}
