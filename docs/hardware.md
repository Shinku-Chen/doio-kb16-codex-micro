# 硬件与兼容性

## 目标硬件

| 项目 | 值 |
|---|---|
| 键盘 | DOIO KB16-01 Rev2 |
| 主控 | APM32F103CBT6（STM32F103 兼容） |
| Flash | 128 KB |
| 按键 | 16 键 |
| 旋钮 | 3 个（左/中/右） |
| 屏幕 | 128×32 单色 OLED |
| RGB | 16 颗 per-key LED |
| 连接 | 有线 USB |

## Rev1 与 Rev2

KB16-01 有两个硬件版本，正常模式下都报告相同的 USB VID/PID，无法仅凭 USB 描述符区分：

| Bootloader 现象 | 版本 | 主控 | 刷写工具 |
|---|---|---|---|
| `dfu-util -l` 显示 `[1eaf:0003] STM32duino bootloader` | **Rev2** | STM32F103 / APM32F103 | `dfu-util` |
| `dfu-util -l` 无输出；`dfu-programmer atmega32u4 get` 有响应 | Rev1 | ATmega32U4 | `dfu-programmer` |

**只支持 Rev2。** Rev1 刷 STM32 固件不会正常运行，严重时会导致设备无法使用。

## USB 身份

| 模式 | VID | PID | 说明 |
|---|---|---|---|
| 原厂 VIA | `0xD010` | `0x1601` | 官方 VIA 3.0 固件 |
| Codex Micro | `0x303A` | `0x8360` | 自定义固件伪装成 Work Louder Codex Micro |

Codex Micro 模式：

- Raw HID usage page：`0xFF00`
- Raw HID usage：`0x0001`
- Report ID：`6`
- Report size：64 字节

## Bootloader

- Rev2 使用 STM32duino / Maple bootloader。
- 用户确认这台设备需要拆开外壳，使用 PCB **内部 `RESET`** 进入刷写流程；先断 USB 再拆机，板子置于绝缘面，接回 USB 后先启动刷写等待，再短按内部 RESET 并松开。2026-10-30 已捕获唯一 `1EAF:0003`、Maple alt 2 并刷入流水线 `0.4.8-qmk`，新版已应答。旧日志中的 RET 称呼保留为历史，不代表外壳有外露复位键，以内部 RESET 实物为准。
- QMK Rev2 文档另列 Bootmagic（按住矩阵 `[0,0]` 后插 USB）及 `QK_BOOT`。Bootmagic 会重置 EEPROM，不作为保留 VIA 键位时的首选；按键布局里也必须实际配置了 `QK_BOOT` 才能使用该动作。
- 刷写命令（QMK 文档）：

```bash
dfu-util -w -a 2 -d 1EAF:0003 -D <firmware.bin>
```

alt 2 必须对应 `Upload to Flash 0x8002000`，不写 Bootloader。刷完核验版本，再真正断电 5 秒后正常插回；此次不按 RET、内部 RESET 或其他键，不开 VIA，核对 10 秒等待页与 Codex 1 秒退出。完整步骤见 `usage.md`。

## 2026-10-29 实机核对

在键盘连接、原厂 VIA 3.0 固件运行的情况下完成：

| 项目 | 结果 | 依据 |
|---|---|---|
| USB 身份 | `D010:1601`，DOIO | macOS USB 枚举 |
| 矩阵左上角方键 | `row 0, col 0` | VIA matrix-state 实测 |
| 左上旋钮按下 | `row 0, col 4` | VIA matrix-state 实测 |
| 右上旋钮按下 | `row 1, col 4` | VIA matrix-state 实测 |
| 下方旋钮按下 | `row 2, col 4` | VIA matrix-state 实测 |
| 左上旋钮 CW | `0x6F` 亮度+ | consumer HID 实测 |
| 右上旋钮 CW / CCW | `0xE9` 音量+ / `0xEA` 音量− | consumer HID 实测 |
| 下方旋钮 CW | `0xB5` 下一首 | consumer HID 实测 |
| HSE | 8 MHz | QMK STM32duino `board.h` 的 `STM32_HSECLK`，原厂固件 USB 已工作 |
| OLED 方向 | 与原厂 rotation 0 一致 | QMK `oled_driver.c` 的 `0xA1/0xC8`、`COM_PINS_SEQ` |
| WS2812 顺序 | LED 0..15 按矩阵 row-major | QMK `kb16/info.json` 的 `rgb_matrix.layout` |

结论：矩阵、三个旋钮的按下位置和旋转方向已经用原厂固件实测确认；HSE、OLED 初始化方向和 WS2812 逻辑顺序由原厂 QMK 定义确认。OLED 实际显示方向、WS2812 实际灯序和自定义固件的 HSE 仍需在刷入 `kb16-fw` 后做一次目视/枚举确认。

## 不支持

- KB16-01 Rev1（ATmega32U4）
- KB16B-02 无线版（USB `FEED:6060`）
- 其他 DOIO 型号（KB03/KB04/KB09/KB12/KB19/KB30/KB38 等）

## 刷写安全

- 刷前确认硬件版本和 Bootloader。
- 保留官方 VIA 3.0 固件作为回退镜像。
- 核对 Release 提供的 SHA-256。
- 如果备份时遇到读保护，停止，不要解除保护或量产擦除。
