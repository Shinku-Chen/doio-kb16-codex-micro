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
- 进入方式：拔掉 USB，按住左上角键，再插上 USB；此时键盘不输入。
- 刷写命令（QMK 文档）：

```bash
dfu-util -a 2 -d 1EAF:0003 -D <firmware.bin>
```

刷完后拔插一次进入新固件。

## 不支持

- KB16-01 Rev1（ATmega32U4）
- KB16B-02 无线版（USB `FEED:6060`）
- 其他 DOIO 型号（KB03/KB04/KB09/KB12/KB19/KB30/KB38 等）

## 刷写安全

- 刷前确认硬件版本和 Bootloader。
- 保留官方 VIA 3.0 固件作为回退镜像。
- 核对 Release 提供的 SHA-256。
- 如果备份时遇到读保护，停止，不要解除保护或量产擦除。
