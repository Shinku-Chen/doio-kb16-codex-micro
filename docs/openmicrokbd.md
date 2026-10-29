# OpenMicroKbd 参考分析

仓库：https://github.com/conol-ai/openmicrokbd  
License：MIT  
硬件：STM32F072CBT6（Cortex-M0，128 KB Flash，16 KB RAM）  
固件：Rust + embassy  

## 一句话

**这是目前最接近你目标的公开实现**：它把 Codex Micro 兼容模式、Work Louder Input 的 `fs.*`、`keymap.json`、`smart_actions.json` 和 keymap 执行引擎都做出来了，而且跑在 128 KB Flash / 16 KB RAM 的 STM32F072 上。

## 它已经实现了什么

`fw/src/codex/` 是完整的 Codex Micro 兼容层：

- USB 身份：Codex Micro 的 VID/PID、`FF00:0001`、Report ID 6、64 字节报告
- `v.oai.hid`：AG/ACT/ENC 事件
- `v.oai.rad`：模拟摇杆四方向
- `v.oai.thstatus`：6 个任务灯的颜色/亮度/effect/speed
- `v.oai.rgbcfg`、`lights.preview`
- `device.status`、`sys.version`
- Work Louder Input 的文件系统 RPC：
  - `fs.list`
  - `fs.readbin` / `fs.writebin`
  - `fs.read` / `fs.write`
  - `fs.delete` / `fs.rmdir`
  - `fs.txbegin` / `fs.txcommit`
- `/fs/keymap.json`：profiles → layers → keymap/encoders/buttons/joystick
- `/fs/smart_actions.json`
- keymap 执行引擎：普通键、媒体、层/Profile、宏、Smart Actions、AppSense
- 13 键 + 1 编码器 + 摇杆 + 触摸的官方模型

它不是“只做了 Codex 灯效”的项目，而是把 Input 的配置协议和 keymap 执行都实现了。

## 资源占用

`fw/README.md` 给出的生产构建数据：

- 68.1 KiB Flash（`DEFMT_LOG=off`）
- 9.6 KiB RAM
- keymap.json 槽：12 KiB
- smart_actions.json 槽：6 KiB
- 芯片：128 KiB Flash / 16 KiB RAM

这说明：**同类 128 KB MCU 可以塞下 Codex 兼容 + Input fs/keymap + smart actions**。KB16 的 STM32F103 有 128 KB Flash / 20 KB RAM，资源上不是完全不可能；但 QMK 本身比 embassy 裸机固件更大，需要实测。

## 关键代码

| 文件 | 作用 |
|---|---|
| `fw/src/codex/mod.rs` | USB 兼容接口、协议总览、消息分发 |
| `fw/src/codex/wire.rs` | JSON-RPC 编解码、`fs.*`、`v.oai.*`、分片/流式写 |
| `fw/src/codex/layout.rs` | keymap.json 解析与执行：profiles/layers/keys/encoders/buttons/joystick/macros/smart actions |
| `fw/src/codex/files.rs` | 两个 Flash 文件槽：keymap.json 12 KiB、smart_actions.json 6 KiB、SHA-1、原子写 |
| `fw/src/codex/sha1.rs` | `fs.list` 的 SHA-1 校验 |
| `layout/src/lib.rs` | Flash 映射、镜像头、bootloader/handoff 协议 |
| `boot/` | 24 KiB resident bootloader，USB HID 更新、CRC 校验、防砖 |
| `app/` | Rust/GPUI 桌面配置器，可作为协议/交互参考 |
| `scripts/test-codex-compat.py` | 端到端兼容测试 |
| `scripts/test-codex-wire.sh` | host 侧协议 shape 测试 |

## 对 KB16 的可用性

### 能直接借鉴的

- `fs.*` 的请求/响应 shape
- `keymap.json` 的完整 schema
- `layout.rs` 的 keycode/action/layer/Profile/smart action 执行逻辑
- `files.rs` 的双槽 Flash 文件存储 + SHA-1 + 原子写
- `wire.rs` 的 64 字节分片、`fs.writebin` 流式写 Flash
- Codex Micro 的 USB 身份、`v.oai.*` 消息、任务灯/灯效
- MIT 许可，保留版权声明即可复用/改写

### 不能直接用的

- MCU 不同：STM32F072（Cortex-M0）vs KB16 APM32F103（Cortex-M3 兼容）
- 固件栈不同：Rust/embassy vs QMK/C
- 外设不同：13 键 + 1 编码器 + 摇杆 + 触摸 vs 16 键 + 3 编码器 + OLED
- Flash 布局/bootloader 不同：OpenMicroKbd 24 KiB resident bootloader vs KB16 STM32duino
- Input 看到的是官方 13 键模型，KB16 多出来的键/旋钮不会出现在 Input 里

## 两条落地路线

### 路线 1：把 OpenMicroKbd 的 codex 模块移植到 KB16

- 保留 KB16 的 STM32F103/QMK 或改用 Rust/embassy
- 把 `wire.rs` / `layout.rs` / `files.rs` / `sha1.rs` 的逻辑移植到 KB16
- 保留官方 13 键模型；KB16 的 13 个键映射到官方位置
- 1 个编码器给 Input；另外两个旋钮做摇杆/层切换
- 触摸用键/旋钮替代；OLED 显示层/任务状态

难点：QMK 的 C 环境要重写 JSON 解析、文件槽、keymap 执行；或者把 KB16 的板级支持移植到 embassy-stm32（APM32 兼容性要验证）。

### 路线 2：直接使用 OpenMicroKbd 硬件

如果目标是“完整兼容 Input + 最少固件工作”，最省事的是：

- 打样/购买 OpenMicroKbd 板（STM32F072，13 键 + 编码器 + 摇杆 + 触摸）
- 直接刷它的固件
- 用 Input 自定义

这比把 KB16 改成官方 13 键模型更自然。

## 建议

- **不要从零写 fs/keymap 协议**：OpenMicroKbd 已经把最难的部分做了，先读它的 `fw/src/codex/`。
- 如果坚持 KB16：优先移植 `layout.rs` + `files.rs` + `wire.rs` 的逻辑，而不是重新发明 schema。
- 如果目标是完整 1:1：考虑直接用 OpenMicroKbd 硬件，KB16 的 16 键/3 旋钮/OLED 会在 Input 模型里被裁剪。
- 先做最小 PoC：`fs.list` + `fs.read` + 最小 keymap.json，确认 Input 能打开；再补 `fs.write`/`txcommit` 和 action 引擎。

## 许可证

OpenMicroKbd 是 MIT。复用代码时保留原版权声明；它是独立开源项目，与 OpenAI/Work Louder 无隶属关系。
