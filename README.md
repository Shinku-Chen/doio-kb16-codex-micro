# DOIO KB16 Codex Micro

**简体中文** | [English](README.en.md)

把 **DOIO KB16-01 Rev2** 变成 Codex Micro 兼容控制器，同时保留 VIA 普通键盘层。基于 QMK，支持 16 键、三个可按旋钮、16 颗 RGB 灯和 128×32 单色 OLED；不需要额外运行本项目的电脑端程序。

> **仅适用于 KB16-01 Rev2 / APM32F103CBT6 / 128 KB Flash / Maple Bootloader。** 不适用于 Rev1（ATmega32U4）、无线版或其他同名硬件。刷写前导出 VIA 配置并保留官方回退镜像；不要擦写 Bootloader 或更改读保护。

## 固件下载与当前版本

- **[v0.4.8-qmk Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases/tag/v0.4.8-qmk)**：固件 `.bin` / `.hex`、VIA 定义和 `SHA256SUMS`。
- 本版：上电 VIA 窗口 **10 秒**；Codex 层左下角 **长按 1 秒**回普通层。灯光、OLED 和键位存储逻辑不因本次改时长而重置。
- `0.4.7-qmk` 已刷入、通过版本应答核验，用户反馈目前正常。`0.4.8-qmk` 的构建及离线验证记录见 [开发文档](docs/development.md)；**新时长版未在本次发布中刷入实机**。
- Git 仓库只保存源码、定义和文档；编译产物只放 Release。升级前仍需备份，不保证任意旧固件的 EEPROM 都兼容。

## 四层：第四层就是 Codex

| VIA 编辑页 / QMK 层 | OLED 空闲显示 | 实际用途 | VIA 改键是否执行 |
|---|---|---|---|
| 第 1 页 / L0 | `1` | 普通键盘 | 是，按键和旋钮均可配置 |
| 第 2 页 / L1 | `2` | 普通键盘 | 是 |
| 第 3 页 / L2 | `3` | 普通键盘 | 是 |
| **第 4 页 / L3** | Logo + `CODEX`，随后任务状态 | **固定 Codex 层** | **否：该页可编辑保存，但运行忽略其内容** |

没有第五层。VIA 的 `0/1/2/3` 是从零开始的索引，屏幕普通层的 `1/2/3` 是便于阅读的编号。

### 启动、改键与切层

```text
真正断电后插入 USB → 显示 VIA WAITTING
  ├─ 10 秒内收到有效 VIA 命令 → VIA 身份锁定，直到断电
  └─ 10 秒内没有 VIA 命令 → 自动复位 → Codex 身份，默认 L3
       ├─ L3 左下角长按 1 秒 → L0（普通键盘）
       └─ L0 / L1 / L2 中按配置为 TO(3) 的键 → L3（Codex）
```

**开机 10 秒内用 VIA 改键：**

1. 提前在支持 WebHID 的 Chromium 浏览器打开 [VIA](https://usevia.app)，在设置中启用 **Show Design tab**。
2. 在 Design 页载入 [`firmware/via/kb16_codex_via.json`](firmware/via/kb16_codex_via.json)（也可从 Release 下载）。
3. 键盘断电重插，看到 `VIA WAITTING` 后，**10 秒内授权 `DOIO / KB16-01` 并让 VIA 成功读取键位**。单纯打开网页或串口连接不算有效 VIA 命令。
4. 一旦成功通信，VIA 身份会一直保持，不必在 10 秒内改完。编辑 **L0–L2**，旋钮方向及按压也可改；配置保存在键盘上，关闭 VIA 仍保留。
5. **建议在每个普通层预留一个 `TO(3)` 返回键**。VIA 选择层动作 `TO(3)`；若需 Any 自定义 QMK 键值，填 `0x5203`。这是“进入第四层”，不是编辑第四层的 Codex 布局。
6. 改完后断电重插，关闭 VIA 或断开其连接，不发送 VIA 命令；等待 10 秒，设备进入 Codex 身份。此时长按左下角 1 秒进入 L0，普通层继续使用已保存的配置。

**Codex → 普通层：** L3 的 **R4C1（左下角）按住 1 秒**后松开，进入 L0，屏幕显示 `1`；短按不切层。普通层不强占这个键，按你的 VIA 配置执行。

**普通层 → Codex：** 按预设的 `TO(3)`。全新默认配置中右上旋钮 B 的按压可从 L0→L1→L2→L3；已有配置可能不同，以你保存的键位为准。

> **USB 身份不等于键盘层。** 切层不会重新枚举 USB。锁定 VIA 身份时，即使切到 L3，Codex 软件也不会获得专用接口；必须断电重插并让 10 秒窗口超时。Codex 身份下可以使用 L0–L2 已保存的键位，但不能当场连接 VIA 改键。

| USB 身份 | VID:PID | Raw HID | 用途 |
|---|---|---|---|
| VIA | `D010:1601` | `FF60:0061`，32 字节，无 Report ID | 浏览器改键 |
| Codex | `303A:8360` | `FF00:0001`，Report ID `6`，64 字节 | Codex Channel 2 事件与灯光 |

## Codex 键位对应表

面向键盘，R/C 从 1 开始；仅描述 **L3 固定布局**，普通层以 VIA 为准。

```text
       C1       C2       C3       C4
R1     空       AG00     AG01     空
R2     AG02     AG03     AG04     AG05
R3     ACT06    ACT07    ACT08    ACT09
R4     回 L0    ACT10    ACT11    ACT12
       长按1秒

右侧：A 左上旋钮 = 摇杆 X；B 右上旋钮 = 摇杆 Y
      C 下方大旋钮 = 原厂编码器
```

| KB16 位置 | Codex 事件 / 用途 | LED（从 0 开始） |
|---|---|---|
| R1C1 / R1C4 | 空位，不发输入 | 0 / 3，始终灭 |
| R1C2 / R1C3 | AG00 / AG01 | 1 / 2 |
| R2C1 / R2C2 / R2C3 / R2C4 | AG02 / AG03 / AG04 / AG05 | 4 / 5 / 6 / 7 |
| R3C1 / R3C2 / R3C3 / R3C4 | ACT06 / ACT07 / ACT08 / ACT09 | 8 / 9 / 10 / 11 |
| R4C1 | 本地切层：按住 1 秒进入 L0，不发送 AG/ACT | 12 |
| R4C2 / R4C3 / R4C4 | ACT10 / ACT11 / ACT12 | 13 / 14 / 15 |

| 旋钮 | 逆时针 | 顺时针 | 按下 |
|---|---|---|---|
| A（左上） | `JOY_LEFT`，摇杆左 | `JOY_RIGHT`，摇杆右 | 无动作 |
| B（右上） | `JOY_DOWN`，摇杆下 | `JOY_UP`，摇杆上 | 无动作 |
| C（下方大旋钮） | `ENC_CC` | `ENC_CW` | `ENC_SW` |

摇杆是离散方向事件模拟，不是连续二维模拟轴。ACT10/ACT11 对应原厂 MIC 的两个半键，可以在支持的宿主里合用为麦克风，也可拆分为其他动作；**没有独立的第 17 个 MIC 键**。宿主内修改 AG/ACT 的功能绑定，不会改写这张固件物理事件表，也不等于 Work Louder Input 配置。

## 小屏幕展示

以下由**当前固件测试实际输出的 128×32 帧缓冲**生成，不是实机照片。所有画面保留左右至少 6 px、上下至少 4 px 安全边距，不显示 DOIO 开机页。

**上电等待：** `VIA WAITTING`（保留指定拼写），最多 10 秒；VIA 锁定后提前结束等待页。

![上电 VIA WAITTING](docs/assets/oled-via-waiting.svg)

**普通层空闲：** L0 / L1 / L2 分别居中显示 `1` / `2` / `3`。

![普通 L0 层号 1](docs/assets/oled-via-1.svg)
![普通 L1 层号 2](docs/assets/oled-via-2.svg)
![普通 L2 层号 3](docs/assets/oled-via-3.svg)

**普通层按键时：** 临时替换层号，显示实际发出的 HID 键名/组合（如 `GUI+C`）、媒体或鼠标输出；按住保留，松开约 900 ms 回层号。显示的不是输入法文字或应用最终动作，不保存按键日志。

![实际输出 GUI+C](docs/assets/oled-via-shortcut.svg)

**进入 Codex：** 终端 Logo + `CODEX` 约 1 秒，随后显示六个 AG 灯的状态估计。

![Codex 进入画面](docs/assets/oled-codex-logo.svg)
![六个 AG 的任务状态估计](docs/assets/oled-codex-tasks.svg)

| OLED 标签 | 灯色推断 | 注意 |
|---|---|---|
| `IDLE` | 白 / 灰 | 空闲外观约定 |
| `RUN` | 蓝 | 常见运行提示 |
| `NEW` | 绿 / 海绿 | 常见完成未读提示 |
| `WAIT` | 橙 / 黄 | 等待审批或回复，不能区分二者 |
| `ERR` | 红 | 常见错误提示 |
| `OTHR` | 青 / 紫 / 粉等 | 不猜测业务含义 |
| `--` | 未分配或明确 off | 不只是呼吸最低点或亮度为 0 |

协议提供灯光参数，**并不提供可靠的任务业务状态**。上述标签只是通用色类推断：例如红色预览也可能显示 ERR。OLED 不显示真实任务标题、额度或 token；扩展灯槽也不会增加当前六格任务页。

## 灯效、颜色与亮度

**宿主下发原色，不设置 GPT/Cindy 专属色表。** 下表 `B` 为宿主指定的亮度；`100% × B` 就是原亮度，不是强制满亮。颜色代表常见宿主约定，不保证自定义主题有相同含义。

### AG 任务灯

| 常见状态 / 下发 | 颜色 | KB16 样式 | 输出亮度 |
|---|---|---|---|
| Idle，白/灰 `solid` | 保留宿主白/灰色调 | 常亮 | **5% × B** |
| 运行 `solid` | 宿主蓝色 | 常亮 | B |
| 完成未读 `solid` | 宿主绿 / 海绿 | 常亮 | B |
| 等待审批 / 回复 `solid` | 宿主橙 / 黄 | 常亮 | B |
| 错误 `solid` | 宿主红色 | 常亮 | B |
| 任意状态 `breath` | 宿主原色 | 呼吸，跟随速度 | 0–100% × B |
| 任意状态 `snake` | 宿主原色 | **跑马映射为呼吸**，跟随速度 | 0–100% × B |
| `shallowBreath` | 宿主原色 | 浅呼吸 | 50–100% × B |
| `rainbow` / `gradient` | 动画生成变化色 | 色相循环 / 渐变 | B |
| `off` / B=0 | 无光 | 熄灭 | 0 |

白/灰呼吸不叠加 Idle 的 5% 系数。速度 0 暂停动画当前相位。数字效果 `0/1/2/3/4/5/6` 依次为 `off/solid/snake/rainbow/breath/gradient/shallowBreath`，也接收对应字符串；当前 rainbow/gradient 使用同类色相循环实现，不承诺原厂逐帧动画一致。

### 第四排环境灯、ACT 和空位

| 场景 / 下发 | 灯位 | 颜色 | 样式 / 亮度 |
|---|---|---|---|
| 蓝色系 `snake`，通常为当前任务运行 | 第四排全部四灯 | 保留蓝色 | **同步呼吸 0–10% × B** |
| 海绿色系 `snake`，通常为录音 | 第四排全部四灯 | 保留海绿 | **常亮 B**，不闪烁 |
| 白/灰或其他颜色 `snake`，如语音处理 | 第四排全部四灯 | 保留原色 | 同步呼吸 0–100% × B |
| 环境 `solid`，如待发送 | 第四排全部四灯 | 保留原色 | 常亮 B |
| 环境 `breath` / `shallowBreath` | 第四排全部四灯 | 保留原色 | 0–100% / 50–100% × B，不套 snake 例外 |
| 环境 `rainbow` / `gradient` | 第四排全部四灯 | 动画变化色 | B，跟随速度 |
| 环境清除 / off | 第四排 | 无光 | 熄灭；若有任务灯或按住反馈则按其优先级显示 |
| 普通 ACT，无环境或扩展任务 | 对应 ACT | 本地纯白 | 默认灭，按住 100% 白，松开灭 |
| 切层键按住 | R4C1 | 本地纯白 | 按住 100% 白，1 秒后退出 L3 |
| 两个空位 | R1C1 / R1C4 | 无光 | 始终灭 |

环境灯覆盖 R4C1–R4C4，不覆盖六 AG 或第三排。任务运行时环境灯也可能亮，**亮灯不等于正在录音**；蓝/海绿规则基于色类与 snake，不是宿主单独传了“运行/录音”状态。

### 优先级、扩展灯和息屏

- USB 挂起、宿主明确关灯和空闲休眠优先，能屏蔽全部 RGB/OLED 及本地白光，但不阻断按键或切层。
- 在允许显示时：切层键按住白光优先；ACT 上已分配的扩展任务灯 > 第四排环境 > 普通 ACT 按住白光。扩展任务 B=0 仍占灯位，`off` 才释放。
- 固件支持任务灯槽 **0–12**：0–5 对应 AG00–AG05，6–12 对应 ACT06–ACT12 的 LED；**输入仍是 ACT，不会变成 AG06–AG12**。已检查的 Cindy 构建仍只下发六路，十三路需要宿主配套，不能仅凭此固件实现十三任务端到端灯光。
- VIA 普通层按自己的 QMK RGB 配置运行，无 Codex 任务/环境覆盖；**一小时无输入或有效通信，背光与 OLED 同时关闭**。
- Codex 层 OLED/RGB 跟随所有支持灯区的聚合下发状态；全部 off、零亮度或非变色效果全黑时同关，另有一小时无活动兜底。呼吸最低点不算关灯；首次未收到灯态前仍保留屏幕。
- **有效心跳与重复报文都会续时**，但不能推翻宿主明确关灯。空闲超时后，输入或有效通信可唤醒；宿主明确关闭时须重新下发点亮状态。策略取决于当前层，不取决于 USB 身份。

完整分类阈值、参数兼容、优先级及实机验收记录：[灯光表](docs/lighting.md) · [实现边界](docs/lighting-compatibility.md)。

## 与原版 Codex Micro 的差异

| 项目 | 原版形态 / 行为 | 本项目 KB16 |
|---|---|---|
| 硬件 | 编码器 + 摇杆占据顶角，其他位置是功能键 | 16 方键 + 3 旋钮；顶角两键空出，左下角留作切层 |
| 摇杆 | 实体摇杆 | 两个顶部旋钮模拟 X/Y 方向事件，无连续模拟位置 |
| 编码器 | 原厂编码器 | 下方大旋钮 C |
| MIC | ACT10 + ACT11 两半键 | 第四排中间两键，可由宿主合用/拆分 |
| 灯带 / 环境光 | 原厂独立环境灯区域 | 映射第四排四颗键灯；snake 变呼吸，海绿录音外观常亮 |
| 按键灯 | 原厂物理布局对应宿主灯态 | 六 AG + 七 ACT 扩展槽；Idle 常亮 5%，普通 ACT 默认灭、按住白光 |
| 屏幕 | 不依赖 KB16 的 OLED | 增加留边 Logo、六灯状态估计、普通层号及实际输出提示 |
| 普通键盘 / 改键 | 原厂产品自身配置体系 | 三层 VIA + 第四层固定 Codex；10 秒 VIA 身份协商 |
| Work Louder Input | 原厂支持的硬件配置工具 | **不实现 Input 改键、`fs.*`、Mapper / Channel 3** |
| 电量 / 无线 | 取决于原厂设备 | KB16 为 USB 有线，无真实电池遥测；协议兼容应答不等于有电池 |
| 协议完整性 | 原厂固件 | 兼容已实现的 Codex Channel 2，不是原厂固件的完整复刻 |

本项目不是官方固件，不保证所有 App 版本、原厂扩展命令、动画或外设功能完全一致。宿主侧功能绑定与灯态通信依赖支持 Codex Micro 的 App；**普通层不运行 App 也可用**。

## 自行构建与测试

使用**单独的 QMK 源码树**，基线固定为 `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`。需要 QMK CLI、带 newlib 的 Arm GNU Toolchain；离线测试还需 Python 3 和 Clang。不要在有其他修改的 QMK 工作区上直接覆盖 core patch。

```bash
# 先准备好上述固定版本的 QMK 及其 submodules、工具链
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
QMK_HOME="$QMK_HOME" python3 firmware/keymaps/codex_micro_via_v1/tools/run_tests.py
```

产物为 QMK 根目录的 `doio_kb16_rev2_codex_micro_via_v1.bin`。补丁脚本会检查基线，并复制 keymap、USB/VIA/ChibiOS/WS2812 补丁；不要只复制 `keymap.c`。构建、恢复、SHA-256 和验证边界见 [开发](docs/development.md) / [刷写](docs/usage.md)。

再生成本页 OLED 预览（测试输出目录需先创建）：

```bash
mkdir -p /tmp/kb16-oled-previews
KB16_OLED_PREVIEWS=/tmp/kb16-oled-previews python3 firmware/keymaps/codex_micro_via_v1/tools/run_tests.py
python3 firmware/keymaps/codex_micro_via_v1/tools/render_oled_previews.py /tmp/kb16-oled-previews docs/assets
```

## 文档、来源与许可

- [文档索引](docs/README.md) · [双模式](docs/via-dual-mode.md) · [键位](docs/keymap.md) · [OLED](docs/oled.md) · [需求](docs/requirements.md) · [变更日志](CHANGELOG.md)
- 参考：[hcyniubi/doio-kb16-codex-micro](https://github.com/hcyniubi/doio-kb16-codex-micro)、[conol-ai/openmicrokbd](https://github.com/conol-ai/openmicrokbd)、[thompson-vii/kb16_rev2_vial_fw](https://github.com/thompson-vii/kb16_rev2_vial_fw)、[QMK](https://github.com/qmk/qmk_firmware)。
- 项目许可见 [LICENSE](LICENSE)；QMK 及引入的第三方文件遵循各自的许可与原有版权声明，不因仓库根目录的 MIT 文件而变成 MIT。
