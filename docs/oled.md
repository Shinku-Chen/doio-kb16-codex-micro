# OLED 自定义

## 硬件

- 128×32 单色 OLED（SSD1306 类）。
- 一帧 512 字节，1 bit/像素，按页（8 行）打包。
- 当前固件用整屏显示层图标，弹窗时覆盖为文字。

## 当前布局

- 4 个层图标：`codex`、`number`、`navigation`、`system`
- 图标尺寸：24×24，放在 `(5, 4)`
- 标签：5×7 字体，2 倍放大，放在 `(36, 9)`
- 弹窗：两行文字，每行最多 21 个字符

## 改层图标

1. 替换 `firmware/keymaps/codex_micro_configurable_v1_3/assets/icons-24/` 下的 4 张 24×24 单色 PNG。
2. 如需改文字，编辑 `tools/generate_oled_icons.py` 里的 `CODEX`、`NUMPAD`、`NAV`、`SYSTEM`。
3. 运行：

```bash
python3 tools/generate_oled_icons.py
```

4. 重新编译并刷入固件（见 `development.md`）。

## 全屏图案

当前生成器是“24×24 图标 + 标签”，不是任意全屏图。要做全屏图：

- 准备 128×32 单色位图，按页打包成 512 字节。
- 在 `oled_icons.c` 里替换 512 字节数组，或修改 `oled_controller_render()` 调用 `oled_write_raw_P()`。
- 重新编译刷入。

## 动画

可以：准备多帧 512 字节数组，在 `oled_controller_render()` 里按 `timer_read()` 切换。当前固件没有内置动画框架，需要自己加。

## 限制

- 只有 1-bit 单色，没有灰度/彩色。
- 不能运行时上传图片；每次改图案都要重编译刷入。
- 不支持 GIF/视频。
- 默认字体只支持 ASCII；中文需要自己做字模/渲染。
- 128×32 分辨率很小，复杂图案会糊；24×24 图标是当前实践尺寸。

## 官方 VIA 固件

官方 VIA 3.0 的屏幕内容编译在固件里，VIA 改不了。要改需要 QMK 源码，修改 `lib/layer_status/layer_status.c` 或 `keymap.c` 的 OLED 渲染，再重新编译。
