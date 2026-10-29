#pragma once

// PoC defaults. Override with -D... or keymap-level config.
#ifndef POC_VIA_WINDOW_MS
#    define POC_VIA_WINDOW_MS 30000
#endif

#ifndef POC_VIA_IDLE_MS
#    define POC_VIA_IDLE_MS 60000
#endif

#define POC_MODE_MAGIC 0x4344584DUL // "CDXM"
#define POC_VIA_LAYER_COUNT 3
#define POC_VIA_LAYER_OFFSET 1
#define POC_TASK_SLOTS 6
