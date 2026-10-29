# 完整实现 Codex Micro 固件的路线

> 目标：判断如何在 KB16 上尽可能完整地实现 Codex Micro 固件，并说明哪些部分受硬件限制。

## 1. 先看官方固件是什么

`worklouder/cm-v2-fw-releases` 的 v0.6.3 是 **Creator Micro 2 / Codex Micro** 的官方合并固件：

- `firmware_0.6.3_merged.bin`，约 2.1 MB
- 内部是 **ESP32-S3 / ESP-IDF / Arduino / LittleFS**，不是 QMK/STM32
- 包含：
  - `/fs/keymap.json`
  - `/fs/smart_actions.json`
  - `/fs/smart_actions.idx`
  - `activeProfileId`、`profiles`、`layers`、`encoders`、`joystick`、`linkedApps`
  - `v.oai.hid`、`v.oai.rad`、`v.oai.thstatus`、`v.oai.rgbcfg`
  - `fs.list`、`fs.read`、`fs.readbin`、`fs.write`、`fs.writebin`、`fs.delete`、`fs.format`、`fs.chksm`、`fs.txbegin`、`fs.txcommit`、`fs.rmdir`、`fs.list_txs`
  - `sys.version`、`device.status`、`sys.bootloader`、`sys.selftest`
  - `host.focused_app`、`lights.preview`
  - `joystick.calibrate`、`joystick.deadzone`
  - 触摸按钮、后置按钮、电池、蓝牙、6 层/Profile、LittleFS 文件系统

结论：**官方固件不能直接刷到 KB16**。KB16 是 STM32F103/APM32F103，128 KB Flash，没有 ESP32-S3、没有蓝牙、没有电池、没有触摸传感器。

## 2. “完整实现”有两种目标

### 路线 A：KB16 上做协议兼容子集（现实）

保留 KB16 硬件，用 QMK 实现：

- Codex Micro USB 身份和原生事件
- 6 个任务灯
- OLED 层号/任务状态
- VIA/Codex 顺序模式
- Mic ACT10/ACT11 拆分
- 可选的 keymap/action 子集

这条路能覆盖 Codex 日常使用，但不是官方固件的全部能力。

### 路线 B：ESP32-S3 自定义控制器（完整）

如果目标是完整复刻官方固件：

- 用 ESP32-S3（或直接使用 Creator Micro 2 硬件）
- Arduino/ESP-IDF + TinyUSB + LittleFS
- 实现完整 keymap.json/smart_actions.json/RPC
- 支持蓝牙、电池、触摸、6 层/Profile

KB16 的 STM32F103 不具备这些硬件，不能在这块板子上 1:1 完整实现。

## 3. 完整固件的模块清单

### 3.1 USB / HID

- VID/PID `0x303A:0x8360`
- Raw HID usage `0xFF00:0x0001`
- Report ID `6`，64 字节报告
- JSON-RPC over vendor HID
- 可选 BLE HID（仅 ESP32-S3）

### 3.2 OAI Bridge

实现以下方法：

- `sys.version`
- `device.status`
- `v.oai.hid`（键盘 → host：AG/ACT/ENC）
- `v.oai.rad`（键盘 → host：摇杆方向）
- `v.oai.thstatus`（host → 键盘：6 个任务灯）
- `v.oai.rgbcfg`（host → 键盘：背景/按键灯）
- `host.focused_app`（host → 键盘：AppSense）
- `lights.preview`

### 3.3 Keymap / Action 引擎

官方 `keymap.json` 的核心字段：

```json
{
  "activeProfileId": 0,
  "profiles": [ ... ],
  "layers": [ ... ],
  "encoders": [["KC_VOLU", "KC_VOLD", "KC_MPLY"]],
  "joystick": {"type": "JOYSTICK", "sectors": []},
  "linkedApps": [ ... ]
}
```

需要实现：

- Profile / Layer 选择
- 普通键、修饰键、媒体键、鼠标、层切换
- Encoder 三动作（CCW/CW/Press）
- Joystick sectors（方向动作）
- Action / Multi Action
- Smart Actions（插入文本、执行命令、打开 URL/App）
- AppSense `linkedApps` 自动切层

### 3.4 LittleFS / fs RPC

官方用 LittleFS 保存 `keymap.json`、`smart_actions.json`。Input 通过 `fs.*` 读写：

- `fs.list`
- `fs.read` / `fs.readbin`
- `fs.write` / `fs.writebin`
- `fs.delete` / `fs.rmdir` / `fs.format`
- `fs.chksm`
- `fs.txbegin` / `fs.txcommit`
- `fs.list_txs`

没有这些，Work Louder Input 只能识别设备，不能加载/保存配置。

### 3.5 灯光 / OLED

- 6 个 AG 任务灯：颜色/亮度/effect
- 背景/按键灯：`v.oai.rgbcfg`
- 官方 3 个层 LED → KB16 OLED 层号
- 官方无屏幕 → KB16 OLED 可显示任务状态
- 触摸按钮 → KB16 无，用旋钮按下/System 层替代

### 3.6 硬件相关

- 6 层/Profile：官方支持，KB16 QMK 目前 4 层
- 蓝牙/电池：KB16 没有
- 触摸传感器：KB16 没有
- 摇杆：官方模拟摇杆；KB16 用 4 键或 2 旋钮
- 编码器：官方 1 个；KB16 3 个

## 4. 推荐分阶段路线

### Phase 0：抓官方协议

用官方 Codex Micro + HID 抓包/代理，记录：

- 上电握手
- `fs.list` / `fs.read` 的 keymap.json
- `v.oai.thstatus` 的完整字段
- `v.oai.hid` / `v.oai.rad` 的键盘事件
- Input 保存 keymap 的完整流程

只有拿到真实 JSON 和报文，才能避免凭字符串猜 schema。

### Phase 1：KB16 原生 Codex 子集

- USB 身份、AG/ACT/Mic/摇杆/编码器
- 6 任务灯
- OLED 层号/任务状态
- 位置映射 + 3 旋钮
- Mic ACT10/ACT11 拆分

这部分已有上游实现和 PoC 骨架。

### Phase 2：KB16 的 keymap/action 子集

- 把层 1-3 的键位/编码器存成紧凑二进制
- 支持普通键、媒体、鼠标、层切换
- 不做完整 smart_actions（Flash/RAM 不够时优先砍）
- VIA 在线改键

### Phase 3：fs RPC + Input 兼容（可选，工作量大）

- 实现 `fs.list`、`fs.readbin`、`fs.writebin`、`fs.delete`
- 提供最小 `keymap.json`
- 让 Input 能打开 Codex Micro 页面
- 再补 `fs.txbegin`/`fs.txcommit`/`fs.chksm`

> 上游调查已经确认：Input 能识别 KB16，但卡在 `fs.list` 的 `Method not found`。补 fs RPC 是 Input 兼容的关键。

### Phase 4：ESP32-S3 完整路线（如果要 1:1）

- 自定义 ESP32-S3 板或使用 Creator Micro 2
- LittleFS + Arduino/ESP-IDF + TinyUSB
- 完整 keymap.json/smart_actions.json
- 蓝牙、电池、触摸、6 层/Profile
- 完整 selftest/OTA/bootloader

## 5. 资源与风险

- **Flash**：KB16 128 KB；当前 Codex 固件约 49 KB。加 VIA + JSON parser + fs + action 引擎可能接近上限。
- **RAM**：STM32F103 20 KB SRAM；JSON/文件缓冲要非常克制。
- **无外部 Flash**：不能像 ESP32-S3 那样放 2 MB LittleFS。
- **无蓝牙/电池/触摸**：这三项无法在 KB16 上 1:1。
- **Input 兼容**：fs RPC 和 keymap.json schema 是最大工作量。
- **协议稳定性**：官方 OAI 协议是内部接口，ChatGPT 更新可能改变。
- **官方无源码**：release 只有 merged bin；只能抓包/字符串/行为反推，不是源码级复刻。

## 6. 建议

如果目标是“KB16 日常用 Codex Micro”：

1. 先完成 Phase 1 + Phase 2（原生控制、任务灯、OLED、VIA、Mic 拆分）。
2. fs RPC 做最小子集，先让 Input 能读 keymap，不追求 smart actions。
3. 不追蓝牙/电池/触摸/6 层，这些换硬件才有意义。

如果目标是“完整复刻官方固件”：

- 直接走 ESP32-S3 路线，KB16 的 STM32F103 不适合。
- 用官方 release 做协议参考，但需要自己实现固件。
