#include QMK_KEYBOARD_H

#include "poc_config.h"
#include "poc_mode.h"
#include "poc_usb.h"

#include "hal.h"

// The STM32duino bootloader owns BKP->DR10, so the mode flag lives in DR9.
// Without a backup battery the register is lost on power loss, which is what
// gives every cold start a fresh 30 second VIA window.
#if defined(RCC_APB1ENR_BKPEN)
#    define POC_BACKUP_AVAILABLE 1
#    define POC_BACKUP_REG BKP->DR9
#else
#    define POC_BACKUP_AVAILABLE 0
static uint16_t poc_backup_shadow;
#endif

#define POC_FLAG_CODEX 0x4344U

static poc_mode_t current_mode = POC_MODE_VIA;
static uint16_t   mode_flags;
static bool       via_window_started;
static bool       via_seen;
static uint32_t   via_window_start;

static void poc_backup_enable(void) {
#if POC_BACKUP_AVAILABLE
    // The STM32duino bootloader writes its own flag to BKP->DR10 on reset, so
    // the write protection stays off for the whole run instead of being
    // toggled around our own accesses.
    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;
#endif
}

static uint16_t poc_backup_read(void) {
#if POC_BACKUP_AVAILABLE
    poc_backup_enable();
    return (uint16_t)POC_BACKUP_REG;
#else
    return poc_backup_shadow;
#endif
}

static void poc_backup_write(uint16_t value) {
#if POC_BACKUP_AVAILABLE
    poc_backup_enable();
    POC_BACKUP_REG = value;
#else
    poc_backup_shadow = value;
#endif
}

void poc_mode_init(void) {
    // A real power cycle must always start in VIA mode, even if the backup
    // domain happened to survive on residual power.
#if defined(RCC_CSR_PORRSTF)
    bool power_on = (RCC->CSR & RCC_CSR_PORRSTF) != 0;
    RCC->CSR |= RCC_CSR_RMVF;
#else
    bool power_on = true;
#endif

    mode_flags = power_on ? 0 : poc_backup_read();
    current_mode = (mode_flags == POC_FLAG_CODEX) ? POC_MODE_CODEX : POC_MODE_VIA;
    poc_backup_write(mode_flags);
    poc_usb_select(current_mode == POC_MODE_CODEX ? POC_USB_DESC_CODEX : POC_USB_DESC_VIA);
}

void poc_mode_start_window(void) {
    if (current_mode != POC_MODE_VIA || via_window_started) {
        return;
    }
    via_window_started = true;
    via_window_start   = timer_read32();
}

void poc_mode_note_via_command(void) {
    if (current_mode != POC_MODE_VIA || via_seen) {
        return;
    }
    poc_mode_start_window();
    via_seen = true;
}

void poc_mode_enter_codex(void) {
    if (current_mode == POC_MODE_CODEX) {
        return;
    }
    mode_flags = POC_FLAG_CODEX;
    poc_backup_write(mode_flags);
    // soft_reset_keyboard() sets the STM32duino "just uploaded" marker, so the
    // bootloader jumps straight back into the application instead of DFU.
    soft_reset_keyboard();
}

void poc_mode_task(void) {
    if (current_mode != POC_MODE_VIA) {
        return;
    }

    poc_mode_start_window();
    if (!via_seen && timer_elapsed32(via_window_start) >= POC_VIA_WINDOW_MS) {
        poc_mode_enter_codex();
    }
}

poc_mode_t poc_mode_get(void) {
    return current_mode;
}

bool poc_mode_is_codex(void) {
    return current_mode == POC_MODE_CODEX;
}

bool poc_mode_via_waiting(void) {
    return current_mode == POC_MODE_VIA && !via_seen;
}
