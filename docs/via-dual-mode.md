# QMK + VIA 双模式固件（codex_micro_via_v1）

当前源码：`firmware/keymaps/codex_micro_via_v1/`，版本 `0.4.9-qmk`，保留 10 秒窗口及 1 秒长按退出；OLED 独立息屏，无任务灯分配保留 Logo，入层从首个可见帧展示约 1 秒。63 项固件离线检查通过，新版未刷入；实机 `0.4.8-qmk` 的启动/切层验收已通过。详细记录见 [开发](development.md)。

保留环境四灯呼吸、留边的 Codex Logo/任务页、VIA 层号/实际输出提示；USB 和四层结构不变，不包含直接启动双接口方案。

## 四层模型

| VIA 编辑页 | 固件层 | 实际运行内容 |
|---|---|---|
| 第一页 / 0 | Layer 0 | 普通键盘，执行 VIA 改键 |
| 第二页 / 1 | Layer 1 | 普通键盘，执行 VIA 改键 |
| 第三页 / 2 | Layer 2 | 普通键盘，执行 VIA 改键 |
| 第四页 / 3 | Layer 3 | 固定 Codex，忽略本页保存的普通键位 |

- 只定义四层，`DYNAMIC_KEYMAP_LAYER_COUNT=4`、`VIA_VISIBLE_LAYER_COUNT=4`、`POC_CODEX_LAYER=3`；没有可进入的 Layer 4。
- 用户确认第四页只作 Codex 入口：VIA 可编辑、保存和读回第四页的按键及旋钮，但运行时该层始终读 flash 中的静态 Codex 表。它不是第四个普通键盘层，也不能用来改变实际 Codex AG/ACT/编码器位置。
- 从普通层配置 `TO(3)` 即可进入 Codex 层；首次初始化的旋钮 B 按压依次为 `TO(1)`、`TO(2)`、`TO(3)`。
- L3 左下角按住 1 秒退出到 L0（短按不动作），普通层不强占该物理按键。
- OLED：普通 L0–L2 空闲居中显示 1/2/3，实际 USB 输出临时替换数字；L3 进入先显示 Logo+CODEX 约 1 秒，有任务灯分配则六 AG 状态，无任务则保留 Logo，不跟随 RGB 全灭而关屏。USB 挂起/一小时静默仍关闭。全部留边、包含默认层，无旧功能名或 REQUESTED 弹窗，见 `oled.md`。

## 启动流程和 USB 身份

```text
上电 / 插 USB
 ├─ 10 秒内收到有效 VIA 命令 → 锁定 VIA 身份，直到断电
 └─ 10 秒内没有 VIA 命令 → 复位进入 Codex 身份，默认 L3
       ├─ L0–L2：普通键盘 HID，读取 VIA 配置
       └─ L3：AG / ACT / ENC / JOY，固定 Codex 表
```

| | VIA 身份 | Codex 身份 |
|---|---|---|
| VID:PID | `D010:1601` | `303A:8360` |
| 厂商 / 产品名 | DOIO / KB16-01 | Work Louder / Codex Micro |
| Raw HID | `FF60:0061`，32 字节，无 Report ID | `FF00:0001`，Report ID 6，64 字节 |
| Raw HID 用途 | VIA 读写四页配置 | Codex Channel 2 JSON-RPC |

键盘 HID 在两种身份下保留。**切层不会改变 USB 身份**：VIA 已锁定时，进入第四层仍不会向 Codex 发送可用的专用 HID 事件。需要断电重插，不连接 VIA，等待 10 秒进入 Codex 身份后使用 L3。

- 窗口从 `keyboard_post_init_user()` 开始；有效协议版本查询即可锁定，错误长度、未知 ID 和越界层不锁定。
- 模式标志使用 `BKP->DR9`，不得占用 bootloader 的 `DR10`；无电池断电后清除。
- 超时调用 `soft_reset_keyboard()` 重新枚举，不停留 DFU。

## Codex 固定键位（Layer 3）

```text
┌────────┬────────┬────────┬────────┐
│  空    │ AG00   │ AG01   │  空    │   旋钮 A（左上）= 摇杆 X
├────────┼────────┼────────┼────────┤
│ AG02   │ AG03   │ AG04   │ AG05   │   旋钮 B（右上）= 摇杆 Y
├────────┼────────┼────────┼────────┤
│ ACT06  │ ACT07  │ ACT08  │ ACT09  │   旋钮 C（下方）= 编码器
├────────┼────────┼────────┼────────┤
│ 长按1s │ ACT10  │ ACT11  │ ACT12  │
│ 回 L0  │    MIC 两个半键        │
└────────┴────────┴────────┴────────┘
```

- 旋钮 A：逆/顺 = 摇杆左/右；旋钮 B：逆/顺 = 下/上；两者按下无动作。
- 旋钮 C：逆/顺/按下 = `ENC_CC / ENC_CW / ENC_SW`。
- 旋钮方向是脉冲模拟，不是连续模拟摇杆。
- 六个 AG 独立任务灯；普通 ACT 无环境提示时按住白光、平时灭；空位保持灭。环境提示覆盖第四排四灯（切层键、ACT10/ACT11/ACT12），录音海绿常亮、白色处理/蓝色运行同步呼吸，亮度 0–100% 且跟随宿主速度。切层键按住白光保持最高优先，1 秒退出不变；完整规则见 [灯光](lighting.md)。
- 进入 L3 临时启用 Codex 灯光，离开恢复普通层 RGB 开关/效果，不额外写 EEPROM。

## VIA 使用与旧配置升级

1. 刷前先导出 VIA 配置，保留官方回退镜像。
2. 提前在 <https://usevia.app> 启用 Design 并加载 `firmware/via/kb16_codex_via.json`，断电上电后 10 秒内授权 `D010:1601` 并成功读取键位。锁定后可一直编辑，不必在 10 秒内改完。
3. 编辑 L0–L2 的按键、旋钮方向和按压；给需要的返回键配置 `TO(3)`。
4. 第四页也可保存/读回，但运行不执行这些键值；不要用该页改键判断 Codex 实际布局。
5. 写入完成后断电重插，不连接 VIA；10 秒后 Codex 身份默认 L3。长按左下角 1 秒进 L0，普通层继续使用保存的改键。

矩阵为 4×5：16 方键在前四列，三旋钮按压为 `[0,4] [1,4] [2,4]`，`[3,4]` 无实体键。

五层升级为四层时保持旧动态键位、编码器和宏的 EEPROM 起址；用未使用空间保留地址，并非保留第五层。一次性把普通 L0–L2 按键/编码器里目标为旧 L4 的 `TO/MO/DF/PDF/TG/OSL/TT/LT/LM` 改为目标 L3。其他键值、第四页和宏不重置，后续 VIA 编辑不重复覆盖。

这些迁移不替代备份。QMK/VIA 自身的配置有效性判断仍可能在不同构建或恢复配置时重置数据；导入旧五层布局前应调整旧 L4 目标，迁移不会在每次导入后重复执行。

## 构建与 core patch

```bash
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```

基线固定为 QMK `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`。产物仅保存在仓库外；当前构建体积、SHA-256 和验收步骤见 [开发](development.md)。

完整替换 patch 包括 USB 双描述符、ChibiOS Raw HID 报告边界和控制请求队列、VIA 可见层数、WS2812 PWM DMA 事件。VIA 32 字节与 Codex 64 字节在两种身份下独立处理。Codex 接收带 ID 的 64 字节和去 ID 的 63 字节，不包含 Mapper/Channel 3 或 Input `fs.*`。

## 验证边界与恢复

四层版离线已验证第四页可保存但不执行、静态/动态键位及编码器路由、旧 L4 目标迁移、存储地址兼容、实际/默认层和 OLED。22 项检查与构建通过不等于 VIA 网页和新固件实机验收。

历史：`0.2.0-qmk` 已于 2026-10-30 通过 RET/DFU 刷入，四项 RPC 和八组灯光目视通过；该版是五层且录音仍快闪，不能视为本次 L3、OLED 和录音常亮已实测。

官方回退镜像：

```text
/Users/chenzhuyu/Downloads/海康威视/刷固件/2351080960doio-kb16_via（刷3.0）.bin
SHA-256 0d1f21580b81d88f0e9e4a1bb87ae5a7a96f72677d12864651b6a0624cccac01
```

进入 bootloader：断 USB 后拆机，接回 USB 并先启动刷写等待，再短按 PCB 内部 RESET 并松开，捕获 `1EAF:0003`。旧 RET 叫法不代表外壳上有外露复位键。仅按 [使用与刷写](usage.md) 操作，不改 bootloader 或读保护；下载完整后先核验版本，不因复位返回 `74` 就重复下载。正常冷启动验收不按任何复位键，不开 VIA。
