VIA_ENABLE = yes
DYNAMIC_KEYMAP_ENABLE = yes
ENCODER_MAP_ENABLE = yes
RAW_ENABLE = yes
WS2812_DRIVER = pwm

SRC += oled_icons.c oled_input.c codex_micro_protocol.c poc_mode.c poc_usb.c poc_via.c poc_power.c

# Consumed by the QMK core patches, so they must be command line defines.
OPT_DEFS += -DPOC_DUAL_USB
OPT_DEFS += -DPOC_OLED_INPUT
OPT_DEFS += -DVIA_VISIBLE_LAYER_COUNT=4

# Codex RPC bursts can exceed four fragments. Larger queues reduce backpressure;
# packet boundaries are preserved separately in the patched USB receive task.
OPT_DEFS += -DRAW_OUT_CAPACITY=16
OPT_DEFS += -DRAW_IN_CAPACITY=8
