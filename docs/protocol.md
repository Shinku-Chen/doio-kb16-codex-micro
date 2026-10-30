# 协议与数据流

## USB

Codex Micro 模式：

- VID/PID：`0x303A:0x8360`
- Raw HID usage page：`0xFF00`
- Raw HID usage：`0x0001`
- Report ID：`6`
- Report size：64 字节

## 当前通道

```text
ChatGPT Desktop ── Channel 2 ──> KB16 固件 ──> AG / MIC 灯光
VIA（启动窗口） ── VIA Raw ──> KB16 EEPROM ──> L1–L3 普通层 / L0 Codex 五处自定义
```

- **Channel 2**：ChatGPT 与键盘的 JSON-RPC，走 Raw HID Report ID 6。
- 当前 keymap 不实现 Mapper/Channel 3 或 Input `fs.*`。VIA 只在独立的 VIA USB 身份下处理，不与 Codex Channel 2 混用。

Raw HID 中断 OUT 按 USB 报告边界分派，兼容 64 字节带 ID 和 63 字节报告体；控制 `SET_REPORT(Output)` 在中断中入队，主循环处理。错误 ID/类型/长度和满控制队列会拒绝控制请求，不在中断中解析 JSON 或写 EEPROM。

`0.6.0-qmk` 的 `device.status.layer_index` 等于实际 QMK 层 + 1（包含默认层）：L0 Codex→1，L1/L2/L3 普通→2/3/4，与 VIA 页号一致。十秒窗口和 USB 身份仍不随切层改变；VIA 等待/锁定初始普通 L1，Codex 身份初始 L0。流水线版已刷入，sys.version / device.status 返回 0.6.0-qmk，当前 layer_index=1（Codex L0）；用户随后整体反馈正常，不扩大为逐项输入/显示精测，历史 0.4.x 应答记录见 `development.md`。

## Channel 2 方法

| 方法 | 方向 | 作用 |
|---|---|---|
| `sys.version` | host → device | 查询固件版本 |
| `device.status` | host → device | 电量/层/连接状态 |
| `v.oai.thstatus` | host → device | 任务灯 c/b/e/s；本固件支持 id 0–12，现有宿主通常仅下发 0–5 |
| `v.oai.rgbcfg` | host → device | 键盘/环境灯配置 |
| `v.oai.hid` | device → host | AG/ACT/ENC 按键事件 |
| `v.oai.rad` | device → host | 摇杆方向事件 |

## 任务灯

`v.oai.thstatus` 的 `params` 是数组，每个槽：

```json
{"id":0,"c":16777215,"b":0.5,"e":1}
```

- `id`：本固件支持 0–12；0–5 对应 AG00–AG05，6–12 映射 ACT06–ACT12 的物理灯，不改变 HID 输入。后七路是 KB16 扩展，已安装 Cindy 仍只生成前六路，详细映射见 [兼容性评估](lighting-compatibility.md)。
- `c`：RGB 颜色整数。
- `b`：亮度 0–1。
- `e`：本机 SDK 发送数字效果编号 0–6；当前固件同时解析数字和对应字符串。
- `s`：动画速度 0–1，0 为暂停当前相位；AG/环境 snake 映射呼吸并使用此速度，海绿环境 snake 常亮例外不变。省略时保持旧值，初始默认 0.4。

本机 ChatGPT `26.1023.51421` 对选中或标记为 pulsing 的任务发送 `breath`，保留其状态色；其他已分配任务通常使用 `solid`。

SDK 的 ThreadLighting 仅要求 id，其他字段省略表示保持。`0.4.5-qmk` 修复了 off 遭局部更新意外变为 solid 的问题；不再因省略 e 或空对象自动点灯。十三槽完整帧可能超过 1024 字节，RPC 重组容量提高到 2048；每个灯对象仍限 256 字节。扩展任务启用时优先于同键的环境/按压灯态（B=0 也保留占位），e=off 释放后恢复旧规则。keys/backlight、sk/sa、m/magic 等的缺口及必要性见兼容性评估，不宣称已完整实现。

`0.4.6-qmk` 的完整宿主 RPC（有合法 method 字符串）及有效 VIA 命令都续时，包括 sys.version/device.status 心跳、重复 ID 或重复灯态内容；只收到坏帧/无 method 不续时。一小时没有输入且没有有效通信时 OLED/RGB 同关。Codex 下已保存的全部受支持任务/环境灯态为 off/零亮度或非变色效果的黑色时，另有立即同步关灯；心跳不取消关灯，只由新的可发光灯态恢复。呼吸低点不关 OLED，黑色基色的 rainbow/gradient 仍视作亮灯状态。USB 挂起和全局休眠优先于本地按压白光，不阻断按键 RPC；详见 `lighting-compatibility.md`。

> 宿主 c/b/e/s 是原始灯光数据，不是业务状态。`0.4.4-qmk` 的渲染先复制这些参数，再应用明确保留的本地微调，不能回写/累积降亮或把颜色替换成预设值。通用 HSV 分类仅用于 OLED 推断和色类外观规则，不维护 App RGB 白名单；不能保证识别任意主题的任务、录音或品牌预览语义。用户保留白/灰系 AG Idle 常亮 5%，并改为 AG 任意颜色 snake→原色 0–100%×B 呼吸，跟随宿主速度，白灰呼吸不叠加 Idle 降亮；其余按协议效果和速度渲染，环境映射第四排四灯且例外不变。完整阈值、输出例外和覆盖关系见 [灯光规则](lighting.md)。

### 状态颜色与此前固件改色（历史）

以下记录修改前的改色，用于定位曾经的色差，不是当前输出规则。状态含义对照 [OpenAI 官方说明](https://learn.chatgpt.com/docs/features/codex-micro)。精确 RGB 值来自本机 `26.1023.51421` 的 `src-D0VLwrLy.js` 中 `hn()`，不是官方文档承诺的固定协议色值。

| 状态 | Codex 下发 RGB | 修改前写入 RGB | 历史差异 |
|---|---|---|---|
| 空闲 `idle` | `#FFFFFF` | `#D7D3FF` | 白色改为偏紫白 |
| 运行 `working` | `#304FFE` | `#1A48AB` | 亮蓝改为较暗蓝 |
| 完成未读 `unread` | `#00FF4C` | `#1FAB21` | 亮绿改为较暗绿 |
| 等待审批/回复 | `#FF6D00` | `#C76700` | 橙色变暗 |
| 错误 `error` | `#FF0033` | `#C7144D` | 红色偏玫红且变暗 |
| 未分配 `off` | `#000000` | `#000000` | 熄灭 |

旧四色映射来自上游 `display_thread_color()`，白色另外由 `CODEX_MICRO_COOL_WHITE_*` 替换；当前源码已删除这两种处理。它们是主动改色，不是 RGB/GRB 排列交换。LED、键帽和视觉适应会影响实物观感，表中数值并非实测光色。

任务颜色落在 AG00–AG05，即 KB16 的 R1C2、R1C3、R2C1–R2C4。ACT 本地按住反馈现使用纯白 `#FFFFFF`，六槽全灭不会再关闭 ACT 反馈；MIC 有环境提示时以该提示优先。

### 灯效与 KB16 支持范围

本机 Work Louder SDK 定义下列效果。协议能力不等于 Codex 会在每个场景使用全部效果。

| 编号 | 效果 | 键盘上的含义 | 当前 KB16 实现 |
|---|---|---|---|
| 0 | `off` | 熄灭 | 支持数字和字符串 |
| 1 | `solid` | 同色常亮 | 能显示常亮 |
| 2 | `snake` | 一段颜色沿灯序移动 | AG 原色 0–100%×B 呼吸；第四排蓝色类为 0–10%×B 呼吸，海绿色类常亮，其他色类为 0–100%×B 呼吸；呼吸跟随宿主速度，0 暂停 |
| 3 | `rainbow` | 循环彩虹色 | 支持，保留宿主速度 |
| 4 | `breath` | 同色明暗呼吸 | 支持数字和字符串，保留宿主速度 |
| 5 | `gradient` | 多颗灯排列为渐变 | 支持，保留宿主速度 |
| 6 | `shallowBreath` | 较浅幅度的呼吸 | 支持，保留宿主速度 |

此前固定 1.6 秒呼吸、忽略宿主速度及 `e:4` 被显示为常亮的问题已修复并通过回归测试。新的波形实现参考 OpenMicroKbd，不声称与原厂逐帧一致；当前已包含在实机 `0.3.0-qmk`，灯效逐帧一致性仍未验证，既有目视结果见 `lighting.md`。

历史宿主录音提示常用海绿跑马、处理语音用白色跑马、准备好用白色常亮；这些是宿主用法，不是固定协议 RGB。源码 `0.4.3-qmk` 对第四排 LED 12–15 的环境 snake 按通用色类作本地调整：蓝色系 0–10%×B 呼吸、海绿色系 B 常亮、其他色系 0–100%×B 呼吸；不替换宿主原色，速度和提示生命周期跟随报文，显式 breath/solid/shallowBreath 不套这些例外。没有独立录音/运行状态字段，色类外观不能当作精确业务状态。keys 不铺满普通 ACT 背景；当前 0.6.0 已取消专用切层白光和固定长按退出，AG/ACT/环境规则不变。

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
- 当前待刷 `0.4.0-qmk` 以 6×8 ASCII 字体和旧 Codex 位图绘制，左右至少 6 px、上下至少 4 px 留黑。L3 进入显示 Logo+CODEX 约 1 秒，然后六个 AG 灯光槽推导的状态；不包含任务标题/额度。
- VIA L0–L2 居中显示 1/2/3，USB 报告成功入队后临时显示实际输出键值，松开约 900 ms 恢复数字；观察 6KRO/NKRO、消费/系统与鼠标，绘制不在 USB 路径执行。无需额外 host 程序，旧通用弹窗不覆盖任务页，见 `oled.md`。

## 兼容性警告

这些协议来自逆向/兼容性调查，不是 OpenAI 公开稳定接口。ChatGPT 桌面版更新后可能改变行为；升级前先验证任务灯、摇杆和 AG/ACT 事件。
