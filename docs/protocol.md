# 协议与数据流

## USB

Codex Micro 模式：

- VID/PID：`0x303A:0x8360`
- Raw HID usage page：`0xFF00`
- Raw HID usage：`0x0001`
- Report ID：`6`
- Report size：64 字节

## 两条通道

```text
ChatGPT Desktop ── Channel 2 ──> KB16 固件 ──> 任务灯 / OLED
Mapper          ── Channel 3 ──> KB16 EEPROM ──> 键位 / 灯光配置
```

- **Channel 2**：ChatGPT 与键盘的 JSON-RPC，走 Raw HID Report ID 6。
- **Channel 3**：Mapper 的二进制配置协议，协议版本 1，340 字节配置，A/B 双槽 + CRC32。

## Channel 2 方法

| 方法 | 方向 | 作用 |
|---|---|---|
| `sys.version` | host → device | 查询固件版本 |
| `device.status` | host → device | 电量/层/连接状态 |
| `v.oai.thstatus` | host → device | 六个任务槽的颜色/亮度/效果 |
| `v.oai.rgbcfg` | host → device | 键盘/环境灯配置 |
| `v.oai.hid` | device → host | AG/ACT/ENC 按键事件 |
| `v.oai.rad` | device → host | 摇杆方向事件 |

## 任务灯

`v.oai.thstatus` 的 `params` 是数组，每个槽：

```json
{"id":0,"c":16777215,"b":0.5,"e":"solid"}
```

- `id`：0–5，对应 AG00–AG05。
- `c`：RGB 颜色整数。
- `b`：亮度 0–1。
- `e`：`off` / `solid` / `breath`。

host 对“当前选中/需要关注”的任务发 `breath`；其他任务通常 `solid`。

> 上游 v1.3.5 只解析 `v.oai.thstatus`；`v.oai.rgbcfg` 仅应答 `{"ok":true}`，没有应用 ambient 颜色。这是“当前任务”改造的入口。

## 摇杆

`v.oai.rad` 发送角度 + 按下状态：

| 方向 | 角度 |
|---|---|
| 右 | `0.00` |
| 下 | `0.25` |
| 左 | `0.50` |
| 上 | `0.75` |

host 只映射四个方向；没有斜向/模拟量。

## OLED

- 128×32 单色，一帧 512 字节。
- `oled_write_raw_P` 写入整帧；层图标为 512 字节数组。
- 弹窗用 QMK 默认 6×8 ASCII 字体，每行最多 21 字符。

## 兼容性警告

这些协议来自逆向/兼容性调查，不是 OpenAI 公开稳定接口。ChatGPT 桌面版更新后可能改变行为；升级前先验证任务灯、摇杆和 AG/ACT 事件。
