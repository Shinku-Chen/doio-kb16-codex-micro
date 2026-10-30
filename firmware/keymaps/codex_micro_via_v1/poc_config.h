#pragma once

// VIA exposes four editable pages; five page-3 positions execute in Codex.
#define POC_CODEX_LAYER 3
#define POC_NORMAL_LAYER_COUNT 3
#define POC_VIA_LAYER_COUNT 4
#define POC_LAYER_KEY_ROW 0
#define POC_LAYER_KEY_COL 0
#define POC_LAYER_KEY_LED 0

// How long the keyboard keeps the VIA USB identity before switching to Codex.
#ifndef POC_VIA_WINDOW_MS
#    define POC_VIA_WINDOW_MS 10000
#endif

// L3 top-left hold exits to L0; VIA layer actions can target layers 0-3.
#ifndef POC_LAYER_KEY_HOLD_MS
#    define POC_LAYER_KEY_HOLD_MS 1000
#endif

#define POC_TASK_SLOTS 6
#define POC_CODEX_IDLE_BRIGHTNESS_PERCENT 5
#define POC_CODEX_RUNNING_AMBIENT_BRIGHTNESS_PERCENT 10
#define POC_IDLE_SLEEP_MS 3600000UL
