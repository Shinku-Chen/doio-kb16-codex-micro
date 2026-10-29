# 使用与刷写

## 前置条件

- DOIO KB16-01 **Rev2**（APM32F103CBT6）。
- macOS 或 Windows；刷写需要 `dfu-util` 或 QMK Toolbox。
- ChatGPT/Codex 桌面版（macOS 14+ / Windows 10+）。

## 刷入 Codex Micro 固件

### 1. 下载与校验

本仓库不存放预编译固件；推荐按 `development.md` 本地构建。如果只想用上游预编译版，可去上游 Release 下载：

- 固件：`doio_kb16_rev2_codex_micro_configurable_v1_3_5_status_colors.bin`

上游 v1.3.5 固件 SHA-256（仅用于上游预编译版）：

```text
B21691424B22950562247E8B36D6307A0E0CCA481221C22592D9EE8ADBA45D0F
```

### 2. 进入 Bootloader

拔掉 USB，按住左上角键，再插上 USB。用 `dfu-util -l` 确认出现：

```text
[1eaf:0003] STM32duino bootloader
```

### 3. 刷写

```bash
dfu-util -a 2 -d 1EAF:0003 -D doio_kb16_rev2_codex_micro_configurable_v1_3_5_status_colors.bin
```

刷完拔插一次。设备应以 `303A:8360` 的 Codex Micro 身份出现。

## ChatGPT / Codex 设置

打开 ChatGPT/Codex 桌面版 → **Settings → Codex Micro**（设备连着才会出现；部分版本在 Settings → Keyboard shortcuts → Accessories）。

可设置：

- **Agent keys（AG00-AG05）**：Pinned / Recent / Priority / Custom；每个键指定 chat、keycap、shortcut 或 skill；单击 vs 双击聚焦。
- **Action keys / Keycaps（ACT06-ACT12、Mic）**：命令、技能、自定义快捷键、插入文本；换 keycap。
- **Analog stick**：四个方向各绑一个命令或技能。
- **Knob**：composer navigation / reasoning / conversation scroll / custom；左转、右转、单击、长按。
- **Microphone key**：push-to-talk 或 Voice Chat；是否拆成两个麦克风键。
- **Brightness**：所有设备灯光亮度。
- **Connection / Battery**：连接与电量；Reset keyboard layout 恢复默认。

> 这些是 Codex 原生层设置。本仓库只生成固件，不包含 Mapper；4 层键位/普通层灯光用 VIA（PoC）或外部上游 Mapper 设置。6 层/AppSense 是官方 Codex Micro + Work Louder Input 的能力，KB16 不支持。

## 改键/改灯

本仓库只负责生成固件，不包含 Mapper。需要图形化改键/改灯时：

- 使用本仓库 PoC 目标中的 VIA 模式（层 1-3）；
- 或使用上游的 Windows Mapper（外部工具，不在本仓库）。

Codex 层（层 0）保持原生控制，不通过 VIA/Mapper 改键。

## 普通层

4 个固定层：Codex、Number、Navigation、System。

- Codex 层：16 个原生控制必须各出现一次；`ACT11` 空槽。
- Number / Navigation / System：16 键 + 9 个旋钮动作可配置。
- 中旋钮按下默认切层；System 层有 OLED 亮度、RGB、Bootloader 等动作。

## OLED

当前按 128×32 单色使用，一帧 512 字节。默认显示 4 个 24×24 层图标 + 标签，弹窗显示 `TASK n`、`NO LINK` 等文字。自定义见 `oled.md`。

## 恢复官方 VIA 固件

如果刷坏了或想回到 VIA：

1. 进入 Bootloader（按住左上角键插 USB）。
2. 刷入官方 Rev2 VIA 3.0 固件（例如 `2351080960doio-kb16_via（刷3.0）.bin`）。
3. 如果 VIA 读不到键位，整片擦除或恢复默认后再写入。

官方 VIA 3.0 固件 SHA-256（当前手头文件）：

```text
0d1f21580b81d88f0e9e4a1bb87ae5a7a96f72677d12864651b6a0624cccac01
```

## macOS 注意

- ChatGPT/Codex 桌面版支持 macOS，并内置 Codex Micro 识别。
- 本仓库不含 Mapper；Mac 上可以用 VIA PoC（浏览器）或外部 Windows Mapper。
- 刷写可用 `dfu-util`；进入 Bootloader 后按上面的命令执行。
