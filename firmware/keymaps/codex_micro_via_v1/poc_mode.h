#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POC_MODE_VIA = 0,
    POC_MODE_CODEX = 1,
} poc_mode_t;

void       poc_mode_init(void);
void       poc_mode_start_window(void);
void       poc_mode_task(void);
poc_mode_t poc_mode_get(void);
bool       poc_mode_is_codex(void);
bool       poc_mode_via_waiting(void);
void       poc_mode_note_via_command(void);
void       poc_mode_enter_codex(void);
