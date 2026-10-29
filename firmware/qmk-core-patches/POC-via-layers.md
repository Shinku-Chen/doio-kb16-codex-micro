# PoC：VIA 层映射（设计稿）

目标：VIA 只编辑层 1-3，层 0 固定为 Codex。

## VIA 侧

- 固件报告 `dynamic_keymap_get_layer_count() = 3`。
- VIA 的 layer 0/1/2 对应固件 layer 1/2/3。
- VIA 的 layer 0 永远不能写固件 layer 0（Codex）。

## 需要改的地方

在 QMK `quantum/dynamic_keymap.c` 增加偏移（或 keymap 级 core patch）：

```c
#define POC_VIA_LAYER_OFFSET 1

uint8_t dynamic_keymap_get_layer_count(void) { return 3; }

uint16_t dynamic_keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col) {
    if (layer >= 3) return KC_NO;
    return dynamic_keymap_get_keycode_raw(layer + POC_VIA_LAYER_OFFSET, row, col);
}

void dynamic_keymap_set_keycode(uint8_t layer, uint8_t row, uint8_t col, uint16_t keycode) {
    if (layer >= 3) return; // 拒绝写 Codex 层
    dynamic_keymap_set_keycode_raw(layer + POC_VIA_LAYER_OFFSET, row, col, keycode);
}
```

编码器同理：`dynamic_keymap_get_encoder` / `set_encoder` 加同一偏移。

## 运行时 keymap

`keymap_key_to_keycode(layer, keypos)`：

- layer 0：用静态 Codex keymap（AG/ACT/摇杆/左旋钮），不走 dynamic keymap。
- layer 1-3：用 dynamic keymap（VIA 写入），偏移见上。

## EEPROM 分区

- Codex A/B 配置：现有 `EECONFIG_USER_DATA_SIZE 712`。
- QMK dynamic keymap：`DYNAMIC_KEYMAP_EEPROM_ADDR` 放到 Codex 用户数据之后。
- 先确认 QMK `EECONFIG_SIZE` 与 `DYNAMIC_KEYMAP_EEPROM_ADDR` 的实际布局，再定地址。

## VIA definition

VIA 模式使用原厂 `0xD010:0x1601`，可以直接加载 KB16-01 官方 definition。

- VIA 报告的层数是 3，界面会显示 3 层。
- 如果 VIA 的灯光页干扰 Codex 灯效，后续再做一份 `lighting: none` 的 definition。
