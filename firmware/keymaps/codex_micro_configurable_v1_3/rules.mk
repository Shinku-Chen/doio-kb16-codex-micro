ENCODER_MAP_ENABLE = yes
RAW_ENABLE = yes
WS2812_DRIVER = pwm

SRC += oled_icons.c codex_micro_protocol.c kb16_config.c kb16_config_transport.c

POC_MODE_SWITCH ?= no
POC_OLED_STATUS ?= no

ifeq ($(strip $(POC_MODE_SWITCH)), yes)
  SRC += poc_mode.c poc_usb.c poc_via.c
  OPT_DEFS += -DPOC_MODE_SWITCH
endif

ifeq ($(strip $(POC_OLED_STATUS)), yes)
  SRC += poc_quota.c poc_oled.c
  OPT_DEFS += -DPOC_OLED_STATUS
endif
