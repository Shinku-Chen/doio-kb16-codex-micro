# Work Louder Input 兼容性方案

> 2026-10-29 范围更新：不再需要 VIA。目标改为「完整兼容 Codex Micro + 能用 Work Louder Input 自定义键盘内容」。

## 结论

**可行，但不是小改。** 这是把官方 ESP32-S3 固件里的 **LittleFS + keymap.json + smart_actions + fs RPC + action 引擎** 搬到 KB16 的 STM32F103/QMK 上。

- 只做 Codex 原生键 + 任务灯 + OLED：可行，已有上游实现。
- 让 Input 能打开并保存 keymap：需要实现 `fs.*` RPC 和 `keymap.json`，工作量大。
- 完整 smart actions / macros / 6 层 / BLE / 电池 / 触摸：KB16 硬件和 128 KB Flash 很可能不够。

## Input 到底需要什么

官方 v0.6.3 固件（ESP32-S3）里能看到这些 RPC：

```text
sys.version
device.status
fs.list
fs.read
fs.readbin
fs.write
fs.writebin
fs.delete
fs.rmdir
fs.format
fs.chksm
fs.txbegin
fs.txcommit
fs.list_txs
lights.preview
host.focused_app
v.oai.hid
v.oai.rad
v.oai.thstatus
v.oai.rgbcfg
sys.bootloader
sys.selftest
joystick.calibrate
joystick.deadzone
```

Input 的配置主要走：

- `fs.list`：列出 `/fs/keymap.json`、`/fs/smart_actions.json` 等
- `fs.read` / `fs.readbin`：读取 keymap
- `fs.write` / `fs.writebin`：保存 keymap
- `fs.txbegin` / `fs.txcommit`：多文件事务
- `fs.chksm`：校验
- `fs.delete` / `fs.format` / `fs.rmdir` / `fs.list_txs`：清理/重置

上游调查已经确认：KB16 能被 Input 识别为 `codex_micro`，但卡在：

```text
method: fs.list
Error: Method not found
```

所以 **fs.list 是第一道门槛**，不是 USB 身份。

## keymap.json 需要的最小结构

官方字符串里能看到：

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

还需要动作/键码：

- 普通键、修饰键、媒体键、鼠标
- 层切换、Profile 切换
- Encoder CCW/CW/Press
- Joystick sectors
- Action / Multi Action
- Smart Actions（插入文本、执行命令、打开 URL/App）
- `linkedApps`（AppSense 自动切层）

完整的 keymap.json 可能几 KB 到几十 KB；KB16 没有外部 Flash，必须存内部 Flash，并用紧凑二进制/受限 JSON 解析。

## KB16 的硬件差异

| 官方 Codex Micro | KB16 |
|---|---|
| 13 键 + 1 编码器 + 摇杆 + 触摸 | 16 键 + 3 编码器，无触摸/无摇杆 |
| ESP32-S3，2 MB Flash，LittleFS | STM32F103，128 KB Flash，无文件系统 |
| BLE + 电池 | 仅有线 |
| 6 层/Profile | 当前 4 层 |

Input 看到的是官方 13 键模型，不会显示 KB16 多出来的 3 个键和 2 个旋钮。

## 两条路线

### 路线 A：KB16 上做 Input 最小兼容（现实）

目标：Input 能打开、读取、保存基本 keymap；不追求 smart actions/macros/6 层全部。

需要：

1. 保持 `303A:8360`、`FF00:0001`、Report ID 6、64B。
2. 实现 `sys.version`、`device.status`。
3. 实现最小 `fs.*`：
   - `fs.list`
   - `fs.read` / `fs.readbin`
   - `fs.write` / `fs.writebin`
   - `fs.delete`
   - `fs.format`
   - `fs.chksm`
   - `fs.txbegin` / `fs.txcommit`
   - `fs.rmdir` / `fs.list_txs`
4. 用内部 Flash 的 A/B 槽保存 `/fs/keymap.json`，不实现完整 LittleFS。
5. 实现一个**受限 JSON 解析器** + **基础 action 引擎**：普通键、媒体、鼠标、层切换；先不做 smart actions/macros。
6. 映射 13 键模型到 KB16 的 13 个键位；多出来的键不用。
7. 1 个编码器映射到 Input 的 encoder；另外两个可做 joystick 或保留。
8. 触摸/电池用占位值（100%），Input 不需要真实电池。

风险：128 KB Flash / 20 KB RAM 很紧；先做 PoC 测量 keymap.json 和解析器的实际大小。

### 路线 B：ESP32-S3 自定义板（完整）

如果目标是 1:1 完整兼容 Input + 6 层 + smart actions + BLE + 电池 + 触摸：

- 用 ESP32-S3（或 Creator Micro 2 硬件）
- Arduino/ESP-IDF + TinyUSB + LittleFS
- 完整 keymap.json/smart_actions.json + fs RPC
- 6 层/Profile、AppSense、OTA/selftest

KB16 的 STM32F103 不适合完整路线。

## 建议的 PoC 顺序

1. **抓官方协议**：用真实 Codex Micro + Input，记录 `fs.list`、`fs.read`、`keymap.json` 的完整请求/响应。
2. **实现 fs.list/read**：先让 Input 能识别并打开 Codex Micro 页面。
3. **提供最小 keymap.json**：6 层/13 键/1 encoder/joystick 的骨架。
4. **实现 fs.write/commit**：让 Input 能保存。
5. **实现基础 action 引擎**：普通键/媒体/层切换；再评估 smart actions。
6. **测 Flash/RAM**：如果超 128 KB，砍 smart actions/macros 或换 ESP32-S3。

## 结论

- **只做 Codex 原生 + 任务灯 + OLED**：可行，已有基础。
- **让 Input 自定义基本 keymap**：可行，但要实现 fs RPC + keymap.json + action 引擎，工作量大。
- **完整官方固件（6 层 + smart actions + BLE + 电池 + 触摸）**：KB16 硬件不够，建议 ESP32-S3。

如果坚持 KB16，建议先做「fs.list + fs.read + 最小 keymap.json」PoC，确认 Input 能打开；这是整个 Input 兼容路线的第一道门槛。
