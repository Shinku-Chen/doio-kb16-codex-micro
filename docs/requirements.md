# DOIO KB16 Codex Micro 固件需求文档

> 状态：PoC 需求基线  
> 适用范围：DOIO KB16-01 Rev2（APM32F103CBT6）  
> 仓库定位：只负责生成固件；不包含 Mapper、不提交预编译固件

## 1. 背景

DOIO KB16-01 是 16 键 + 3 旋钮 + 128×32 单色 OLED 的有线 macropad。目标是让它在 Codex 场景下模拟官方 Codex Micro 的核心控制，同时保留普通键盘/VIA 的可配置能力。

当前工作副本来自上游 `hcyniubi/doio-kb16-codex-micro`，但本仓库是独立固件生成仓库：

- 只保留固件源码、QMK core patch、构建配置和文档
- 不包含 Windows Mapper
- 不提交 `.bin` / `.hex` / `.uf2` 等编译产物
- 默认不依赖任何额外常驻 host 程序

## 2. 目标

1. 在 KB16-01 Rev2 上提供 Codex Micro 兼容的 USB 身份和原生控制。
2. Codex 层（层 0）保持纯 Codex 原生控制，不被 VIA 改写。
3. 层 1-3 作为纯普通层，允许通过 VIA 在线改键。
4. 插电后先进入 VIA 模式；30 秒内收到 VIA 命令则一直保持 VIA，直到断电；30 秒内没有 VIA 命令则切到 Codex。改完/刷完后断电重插。
5. OLED 显示当前层号/层名和六个任务状态。
6. 默认不引入额外 host 程序；额度显示作为默认关闭的可选能力。
7. 仓库只保留可复现的固件源码和构建说明。

## 3. 范围

### 3.1 In scope

- QMK 固件源码（keymap、协议、OLED、配置）
- QMK core patch（USB 描述符、Raw HID、DMA 等）
- 模式切换状态机
- VIA 模式与 Codex 模式的顺序切换
- 层 1-3 的 VIA layer offset 与写保护
- EEPROM 分区
- OLED 层号/任务状态显示
- 构建、刷写、验收文档

### 3.2 Out of scope

- Windows Mapper（本仓库不包含）
- 预编译固件（本仓库不提交）
- 默认额度显示（需要 host daemon，默认关闭）
- 触摸传感器、模拟摇杆、无线、6 层/AppSense
- 在同一层混排 Codex 原生键和 VIA 普通键
- 运行时上传 OLED 图片、彩色 OLED、GIF/视频

## 4. 术语

| 术语 | 含义 |
|---|---|
| Codex 模式 | 设备以 `303A:8360` 出现，ChatGPT 识别为 Codex Micro |
| VIA 模式 | 设备以 `D010:1601` 出现，VIA 识别为 KB16-01 |
| 层 0 | Codex 原生层，固定 16 个逻辑原生控制（12 键 + 3 旋钮） |
| 层 1-3 | Number / Navigation / System，VIA 可编辑 |
| 任务槽 | AG00-AG05 对应的 6 个 Codex 任务灯 |
| 30s 窗口 | 插电后允许 VIA 连接的初始时间 |

## 5. 功能需求

### FR-1 Codex 模式

- 设备以 `VID 0x303A / PID 0x8360` 枚举。
- Raw HID：usage page `0xFF00`，usage `0x0001`，Report ID `6`，64 字节报告。
- 层 0 按 `docs/keymap.md` 的 4×4 **位置对应**：
  - KB16 R1C2/R1C3/R2C1-R2C4 → AG00-AG05
  - KB16 R3C1-R3C4 → ACT06-ACT09
  - KB16 R4C1 → Mic（ACT10）
  - KB16 R4C3 → ACT12
  - KB16 R1C1 / R1C4 / R4C2 / R4C4 → 不使用
- 逻辑上仍发送 16 个原生控制：AG00-05、四方向摇杆（由两个摇杆轴旋钮产生）、ACT06-09、ACT12、Mic；`ACT11` 永不发送。
- Mic 是 `ACT10` 事件，不是特殊硬件；可以与其他原生控制互换，但只能出现一次。官方双宽 Mic 对应 `ACT10_ACT11`；KB16 用单键发 `ACT10`，`ACT11` 空置。
- 3 个旋钮：1 个 Codex 原生编码器（CC/CW/Press）+ 2 个摇杆轴（X/Y）。
- 编码器/摇杆用旋钮的具体物理分配可按手感调整。
- ChatGPT 可通过 `v.oai.thstatus` 驱动 6 个任务灯；选中/需要关注的任务使用呼吸效果。

### FR-2 VIA 模式

- 设备以 `VID 0xD010 / PID 0x1601` 枚举。
- Raw HID：usage page `0xFF60`，usage `0x0061`，32 字节报告。
- 使用原厂 KB16-01 VIA definition，不要求自定义 JSON。
- VIA 只暴露 3 层：VIA layer 0/1/2 对应固件 layer 1/2/3。
- 固件 layer 0（Codex）不暴露给 VIA；VIA 写入 layer 0 必须被拒绝。
- VIA 可编辑普通层的按键和编码器；灯光页是否开放由后续决定。

### FR-3 模式切换

- 上电默认进入 VIA 模式。
- 从上电开始计时 30 秒。
- 30 秒内收到任意 VIA 命令（如协议版本查询）→ 认为 VIA 已连接，保持 VIA 模式。
- 30 秒内没有收到 VIA 命令 → 切换 Codex 模式。
- VIA 已连接后一直保持 VIA，不自动切回 Codex。
- 切换通过写模式标志 + `reset_keyboard()` 重新枚举；也可以后续改为软 USB 重枚举。
- 断电前应确保 VIA 写入完成，避免丢写。

### FR-4 层职责

- 层 0：纯 Codex 原生控制，按 `docs/keymap.md` 的位置映射；不允许 VIA 写。
- 层 1：Number。
- 层 2：Navigation。
- 层 3：System。
- 层 1-3 允许 VIA 改键；层 0 的 Codex 原生键不允许混入普通键。

### FR-5 OLED

- 128×32 单色 OLED，一帧 512 字节。
- 默认显示：
  - 第 1 行：层号 + 层名，例如 `L0 CODEX`
  - 第 2 行：六个任务槽状态，例如 `1R 2W 3. 4U 5E 6-`
  - 第 3-4 行：图例 `R run W wait U new` / `E err . idle`
- 任务状态来自 `v.oai.thstatus` 的颜色/effect，不显示任务标题。
- 支持替换 24×24 层图标和标签；全屏图案/动画通过修改固件实现。
- 不支持运行时上传图片；不支持彩色/GIF。

### FR-6 任务灯

- 6 个 AG 键分别对应 6 个任务槽。
- 每槽颜色/亮度/effect 由 ChatGPT 的 `v.oai.thstatus` 驱动。
- 当前选中/需要关注的任务使用 `breath`；其他任务通常 `solid`。
- 可选增强：当前任务白色呼吸、其他五个变暗；属于固件修改，不影响 VIA 层。

### FR-7 存储

- Codex A/B 配置保存原生键位顺序、灯光/OLED 等设置。
- VIA 的 QMK dynamic keymap 保存层 1-3 键位/编码器。
- 两套数据必须显式分区，不能互相覆盖。
- 重启后两套配置都要保留。

### FR-8 构建与仓库

- 构建目标：`doio/kb16/rev2:codex_micro_configurable_v1_3`。
- 需要 QMK、ARM 工具链和 `firmware/qmk-core-patches/`。
- 仓库不提交 `.bin` / `.hex` / `.uf2` / `.elf` / `.map`。
- 仓库不包含 Mapper。
- `.gitignore` 忽略构建产物和本地 build 目录。

### FR-9 可选额度显示（默认关闭）

- 默认不启用，不需要额外 host 程序。
- 可选 `POC_OLED_QUOTA=yes`：host daemon 调用 Codex app-server `account/rateLimits/read`，算剩余百分比后通过 Raw HID 发 `v.oai.quota`。
- 键盘只显示 `p5` / `p7` / `cr`，不直接联网。
- 该功能不属于默认 PoC 验收范围。

## 6. 非功能需求

- NFR-1 硬件安全：只支持 KB16-01 Rev2；Rev1/无线版不刷。
- NFR-2 刷写安全：刷前备份/保留官方 VIA 3.0 固件；核对 SHA-256；读保护时停止。
- NFR-3 兼容性：ChatGPT/Codex 更新可能改变协议；升级后必须复测。
- NFR-4 资源：128 KB Flash；改动前评估 Flash/RAM；VIA + Codex 协议同时存在时仍要能编译。
- NFR-5 USB：VIA 32B 与 Codex 64B 报告不能互相污染；模式切换要重新枚举。
- NFR-6 macOS：VIA 用浏览器网页；默认功能不需要常驻 host 程序。
- NFR-7 可维护性：协议、层偏移、EEPROM 地址集中定义，附测试/验收步骤。
- NFR-8 文档：构建、刷写、恢复、已知限制必须可复现。

## 7. 架构要点

```text
上电
  └─ VIA 模式（D010:1601，FF60:0061，32B）
       ├─ 30s 内收到 VIA 命令 → 保持 VIA
       ├─ 30s 内无 VIA 命令 → Codex 模式
       └─ VIA 已连接 → 一直保持（断电重插后重新计时）

Codex 模式（303A:8360，FF00:0001，Report 6，64B）
  ├─ 层 0：Codex 原生控制
  └─ 层 1-3：读取同一份 QMK dynamic keymap
```

- `codex_micro_raw_hid_receive()` 处理 Codex Channel 2/3。
- `via_command_kb()` 负责 VIA 命令检测，并把 64B/Report ID 6 的 Codex 报文路由给 Codex。
- `poc_mode.c` 管理 30s 窗口和 `.noinit` 标志。
- `poc_usb.c` 提供描述符选择接口；真正切换由 QMK core patch 实现。
- 层 0 的位置映射见 `docs/keymap.md`：12 个 Codex 键 + 3 个旋钮，左上/右上/右下/下排中间键不使用。
- `poc_oled.c` 渲染层号/任务状态；`poc_quota.c` 可选。

## 8. 验收标准

1. 插电进入 VIA 模式；30 秒内打开 VIA 并连接，设备保持 VIA。
2. 30 秒内没有 VIA 命令，设备自动切到 Codex，ChatGPT 识别为 Codex Micro。
3. VIA 连接后一直保持；断电重插后 30 秒内不连接 VIA，设备切到 Codex。
4. VIA 能读写层 1-3；尝试写层 0 被拒绝，层 0 原生控制不变。
5. Codex 模式下 AG/ACT/Mic 键 + 3 个旋钮（编码器/摇杆 X/Y）事件正常；未使用键不发送 Codex 事件。
6. 6 个任务灯按 `v.oai.thstatus` 显示；选中任务有呼吸效果。
7. OLED 显示层号和六个任务状态；无 host 程序时任务状态显示 `.`。
8. 断电重启后 VIA 层 1-3 键位和 Codex 配置都保留。
9. 仓库中没有 `.bin`/`.hex`/`.uf2`，没有 Mapper；构建产物只在本地。
10. 默认功能不需要额外常驻 host 程序。

## 9. 非目标

- 不追求 1:1 复刻 Codex Micro 的触摸、模拟摇杆、无线、6 层/AppSense。
- 不在同一层混排 Codex 原生键和 VIA 普通键。
- 不把 Mapper 放回本仓库。
- 不默认启用额度显示。
- 不实现运行时 OLED 图片上传或彩色屏幕。

## 10. 风险与开放问题

- `.noinit` 是否被 STM32duino bootloader 保留；不保留则改用 RTC backup。
- 切换 USB 身份后 ChatGPT 是否自动重新识别；可能需要重启 app。
- VIA 写入过程中拔电可能丢写；需要确保写入完成后再断电。
- QMK dynamic keymap 与 Codex A/B 配置的 EEPROM 地址必须实测确认。
- STM32F103 端点/描述符资源是否足够；顺序模式比同时双接口简单。
- VIA 的 encoder/lighting 支持范围需要按 definition 实测。
- 30 秒窗口对首次打开 VIA 偏紧；可改 60 秒或按住键强制 VIA。
- 位置映射需要改 `native_keys`、旋钮动作、校验和任务灯映射；ACT11 位置空置。
- 逆向协议可能随 ChatGPT 更新失效。

## 11. 里程碑

| 阶段 | 内容 | 状态 |
|---|---|---|
| M1 | 仓库清理：移除预编译固件、Mapper，添加 `.gitignore` | 已完成 |
| M2 | PoC 骨架：模式标志、30s 窗口、VIA 命令检测、OLED 层号/任务状态 | 已完成（未编译验证） |
| M3 | USB 描述符切换 core patch + VIA_ENABLE | 待做 |
| M3.5 | 位置映射改造：12 键 + 3 旋钮（编码器/摇杆 X/Y） | 待做 |
| M4 | VIA layer offset + EEPROM 分区 | 待做 |
| M5 | 实机编译/刷写/验收 | 待做 |
| M6 | OLED 自定义图标/任务高亮增强 | 待做 |
| M7 | 可选额度显示（host daemon） | 暂缓 |
