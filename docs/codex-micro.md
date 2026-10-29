# 官方 Codex Micro 与 KB16 对比

## 一句话

官方 Codex Micro 是 13 键 + 触摸 + 模拟摇杆 + 1 旋钮 + 蓝牙的成品；KB16 + Codex 固件是 16 键 + 3 旋钮 + 128×32 OLED 的有线平替。核心 Codex 键位和任务灯能覆盖，但触摸、模拟摇杆、无线和官方更新没有。

## 对比

| 能力 | Codex Micro（官方） | KB16 + Codex 固件 |
|---|---|---|
| 按键 | 13 个机械键 | 16 键（Codex 层 12 键 + 4 方向键） |
| 旋钮 | 1 个 | 3 个（左旋钮锁定 Codex 原生） |
| 摇杆 | 模拟摇杆 | 4 个方向键，无模拟量 |
| 触摸 | 左下角触摸传感器 | 没有 |
| 屏幕 | 没有 | 128×32 单色 OLED |
| 任务灯 | 6 个 agent 灯 | 6 个 AG 灯，host 驱动 |
| 原生键 | AG00-05、ACT06-12、Mic | AG00-05、ACT06-09/12、Mic；ACT11 空槽 |
| 无线 | 蓝牙 + USB-C | 仅有线 |
| 层数 | 最多 6 层 | 4 个固定层 |
| 配置 | ChatGPT 设置 + Work Louder Input | ChatGPT 设置 + Windows Mapper；Input 不可用 |
| 官方支持 | 官方固件/更新 | 非官方、逆向协议，可能随 ChatGPT 更新失效 |

## 原生控制

Codex 层包含 16 个原生控制，必须各出现一次：

- `AG00`–`AG05`：6 个 agent 键
- 摇杆四方向：上/右/下/左
- `ACT06`、`ACT07`、`ACT08`、`ACT09`、`ACT12`
- Mic（`ACT10`）
- `ACT11` 为空槽，不发送

左旋钮固定为 Codex 原生旋钮（`ENC_CC` / `ENC_CW` / `ENC_SW`）；中、右旋钮在普通层可配置。

## 摇杆

Codex 把摇杆当成四方向快捷键，不是鼠标/滚轮：

- 默认：上 = Toggle Plan Mode，右 = Forward，下 = Toggle Sidebar，左 = Back。
- 每个方向可在 `Settings → Codex Micro → Analog direction mappings` 绑定一个命令或技能。
- 斜推会归到最近的主方向；没有斜向/8 方向；按住不会连续触发。
- 官方硬件是模拟摇杆，但 Codex 原生只用四个方向；KB16 只有四个数字方向键。

## 触摸传感器

官方 Codex Micro 左下角是触摸传感器，不是摇杆：

- 用于切换层，最多 6 层。
- 旁边 3 个小 LED 用二进制显示当前层号。
- 配合 AppSense 可按前台 App 自动切层。

KB16 没有触摸传感器；Codex 固件用中旋钮按下/System 层动作切 4 个固定层，OLED 显示当前层图标。

## 能实现 / 不能实现

**已经实现**

- 6 个 agent 键、动作键、Mic、左旋钮、四方向摇杆、6 个任务灯
- 4 个固定层、OLED 层图标、RGB 状态色、Mapper 改键/灯光

**小改固件**

- 当前任务高亮（白色呼吸/其他变暗）
- 自定义 OLED 图标、Logo、弹窗文字、任务状态颜色
- 背景色跟随当前任务（解析 `v.oai.rgbcfg` ambient）

**做不到**

- 电容触摸手势
- 模拟摇杆/斜向/力度
- 无线
- 官方固件更新
- 刷 Codex 固件后继续用 VIA
- 运行时上传图片/GIF/彩色屏幕
