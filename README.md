# DOIO KB16 Codex Micro

**简体中文** | [English](README.en.md)

把 **DOIO KB16-01 Rev2** 变成 Codex Micro 兼容控制器，同时保留 VIA 普通键盘层。基于 QMK，支持 16 键、三个可按旋钮、16 颗 RGB 灯和 128×32 单色 OLED；不需要额外运行本项目的电脑端程序。

> **仅适用于 KB16-01 Rev2 / APM32F103CBT6 / 128 KB Flash / Maple Bootloader。** 不适用于 Rev1（ATmega32U4）、无线版或其他同名硬件。刷写前导出 VIA 配置并保留官方回退镜像；不要擦写 Bootloader 或更改读保护。

## 固件下载与当前版本

> **开发版 `0.6.0-qmk`：第一页 L0 改为 Codex，L1–L3 为普通 VIA 层。取消固定长按退出，五个自定义位置全部直接执行 VIA 配置。** 建议 L0 左下设 `MO(1)`：按住进入普通 L1，松开回 Codex；普通层可配 `TO(0)` 返回。旧四页配置自动调整顺序并转换层动作目标，升级前仍应导出备份。本地验证已通过；正式刷写请使用本提交对应的 main 流水线 Artifact（非 Release），不用本机镜像。本版未刷入，设备仍运行 0.5.0，历史验收不代表本版已实测。详见 [五处改键说明](docs/codex-via-keys.md)。

- **[v0.4.9-qmk Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases/tag/v0.4.9-qmk)**：固件 `.bin` / `.hex`、VIA 定义、`BUILD-INFO.json` 和 `SHA256SUMS`。
- 本版修复任务灯全灭导致 OLED 黑屏：**屏幕独立息屏，无任务灯分配时保留 Logo+CODEX，进入 Codex 先展示完整 1 秒 Logo**。背光、键位存储、10 秒 VIA 窗口及 1 秒退出不变。`0.4.9-qmk` 流水线固件已刷入，版本和 Codex 入层 Logo 已实机确认；一小时息屏及按键唤醒按用户选择跳过，尚未实测。
- `0.4.8-qmk` 流水线固件已刷入，版本应答及实机验收通过：正常插电约 10 秒等待页、短按不切层、长按约 1 秒回 L0、返回 Codex 均正常；主机同时捕获四层切换，不把手按时长当作仪器精测。详见 [开发文档](docs/development.md)。
- Git 仓库只保存源码、定义和文档；编译产物只放 Release。升级前仍需备份，不保证任意旧固件的 EEPROM 都兼容。
- **正式固件由 [GitHub Actions](https://github.com/Shinku-Chen/doio-kb16-codex-micro/actions/workflows/firmware.yml) 从标签源码生成和发布，不使用本机编译附件。** `BUILD-INFO.json` 记录源码提交、QMK/编译器版本、流水线链接及资产摘要，最终校验和见 `SHA256SUMS`；0.4.8 已经 [流水线重建并替换附件](https://github.com/Shinku-Chen/doio-kb16-codex-micro/actions/runs/41041000839)，原标签保持不动。

## 刷机：拆机按内部 RESET

**本项目这台 KB16-01 Rev2 需要拆开外壳，按 PCB 内部的 `RESET` 按键进入刷写流程。旧记录中的 `RET` 叫法不代表外壳外面有可直接操作的复位键，以内部 RESET 实物为准；普通键和 VIA 切层不能代替硬件复位。**

1. 先导出 VIA 配置，准备官方回退镜像，下载流水线 Release 的全部附件（BIN/HEX、VIA 定义、`BUILD-INFO.json`、`SHA256SUMS`）并核对来源/哈希。
2. **断开 USB 后再拆外壳**，露出 PCB 内部 `RESET`。将板子放在绝缘表面，避免金属工具短接；不要操作 BOOT/SWD 焊盘或改读保护。
3. 接回 USB，在电脑先启动刷写等待，然后**短按内部 `RESET` 并松开**，捕获 Maple DFU `1EAF:0003`。目标必须是 alt 2 的 `Upload to Flash 0x8002000`；只写应用区，不写 Bootloader。

```bash
# 在固件文件所在目录执行；命令先等待设备，再按内部 RESET
dfu-util -w -a 2 -d 1EAF:0003 \
  -D doio_kb16_rev2_codex_micro_v0.4.9-qmk.bin
```

4. 下载期间不要拔线或再按复位。若只重启、没捕获 DFU，先确认等待命令仍在，再重新尝试内部 RESET；出现 `UNKNOWN` / `LIBUSB_ERROR_PIPE` 则停止，断电重新进入，不做全片擦除或解保护。
5. 下载完成后核验重新枚举及版本，**不要仅因复位阶段返回 `74` 就重复刷写**。BIN 大小以本版 `BUILD-INFO.json` 为准，其中包含 16 字节 DFU suffix，下载主体不含 suffix。HEX 不是原始 BIN，不能传给上述命令。
6. **验证正常开机时不按任何复位键**：拔 USB 等 5 秒，正常插回，不按内部 RESET 或其他键，不开 VIA。应显示 `VIA WAITTING` 约 10 秒，随后进入 Codex。注意下载的 0.4.9 是旧 L3/左下长按退出模型；开发版 0.6.0 才是 Codex L0、自配层切换，不能混用说明。

首次刷写与回退的完整步骤、识别失败处理见 [使用与刷写](docs/usage.md)。屏幕文字和手按时长需要实机目视确认，USB 枚举计时只是辅助证据。

## 四层：第一层就是 Codex

| VIA 编辑页 / QMK 层 | OLED 空闲显示 | 实际用途 | VIA 改键是否执行 |
|---|---|---|---|
| **第 1 页 / L0** | Logo/任务页，自定义输出临时替换 | **原生 Codex + 五处 VIA 自定义** | 仅五处；原生 AG/ACT、旋转和 C 按下固定 |
| 第 2 页 / L1 | `2` | 普通键盘 | 按键和旋钮均可配置 |
| 第 3 页 / L2 | `3` | 普通键盘 | 是 |
| 第 4 页 / L3 | `4` | 普通键盘 | 是 |

没有第五层。VIA 索引从零开始；第一页就是 L0。普通页的屏幕数字对应编辑器页号 2/3/4。

### 启动、改键与切层

```text
真正断电后插入 USB → VIA WAITTING（普通 L1）
  ├─ 10 秒内收到有效 VIA 命令 → VIA 身份锁定，直到断电
  └─ 10 秒内没有 VIA 命令 → 自动复位 → Codex 身份，默认 L0
       ├─ 自配 MO(1) → 按住使用普通 L1，松开回 Codex L0
       └─ 自配 TO(1/2/3) → 普通层；TO(0) → Codex
```

**开机 10 秒内用 VIA 改键：**

1. 提前在支持 WebHID 的 Chromium 浏览器打开 [VIA](https://usevia.app)，启用 **Show Design tab**，载入 [VIA 定义](firmware/via/kb16_codex_via.json)。
2. 键盘断电重插，看到 `VIA WAITTING` 后，**10 秒内授权 `DOIO / KB16-01` 并让 VIA 成功读取键位**。只打开网页或串口不算命令；成功后一直锁定，不必十秒内改完。
3. 第一页 L0 只修改五个自定义位置；普通整层在第二至第四页 L1–L3 修改。配置保存在键盘 EEPROM，关闭 VIA 仍保留。
4. 推荐第一页左下角设 **`MO(1)`（Any：`0x5221`）**，按住进入普通 L1，可同时按该层其他键，松开回 Codex。也可用 `TO(1)` 常驻普通层；给普通层预留 **`TO(0)`（Any：`0x5200`）** 返回 Codex。
5. **取消固定长按退出**：左上角也直接发键，不再等待松开、不再长按抢占、不再叠加退出白光。五处默认无动作，请自行保留切层入口；配置错误可重新在开机 VIA 窗口修正。
6. 改完后断电重插，关闭 VIA，不发送命令；等待十秒，Codex USB 身份默认进入 L0。不需要额外程序。

`MO(n)` 保留 QMK 标准的“高层优先”规则，不是强制切到任意低层；Codex 放在最低 L0 后，向普通 L1–L3 临时切层可自然生效。普通层的透明键继承普通基底 L1，不会漏出 L0 的 AG/ACT；要回 Codex 用 `TO(0)`。

**升级迁移：** 旧 L3→新 L0，旧 L0/L1/L2→新 L1/L2/L3；键位、旋钮映射及层动作目标一并调整。原 Codex 左下 `MO(0)` 自动变为 `MO(1)`，0.5.0 已设置的五处不再清空，宏数据及 EEPROM 地址不变。不要直接导入未转换的旧顺序备份；先检查页序与目标层。

> **USB 身份不等于键盘层。** 切层不重新枚举 USB；VIA 锁定时切到 L0 也没有可用 Codex 专用接口。须断电重插，让十秒窗口超时。Codex 身份下 L1–L3 使用已保存的 VIA 配置，但不能当场在线改键。

| USB 身份 | VID:PID | Raw HID | 用途 |
|---|---|---|---|
| VIA | `D010:1601` | `FF60:0061`，32 字节，无 Report ID | 浏览器改键 |
| Codex | `303A:8360` | `FF00:0001`，Report ID `6`，64 字节 | Codex Channel 2 事件与灯光 |

## Codex 键位对应表

面向键盘，R/C 从 1 开始；描述 **0.6.0 开发版 L0 / 第一页**，普通层以 VIA 为准。VIA1–5 是可改位置标记，不是层号；已发布 0.4.9 尚不支持这五处。

![KB16 Codex L0 键位与三个旋钮](docs/assets/kb16-codex-layout-zh.svg)

矢量图可放大查看：[中文 SVG](docs/assets/kb16-codex-layout-zh.svg) · [English SVG](docs/assets/kb16-codex-layout-en.svg)。键位与旋钮中心位置按当前固件/VIA 定义绘制，外观非等比例；图中配色只区分按键类型，不代表实际灯色。完整说明见 [键位图](docs/keymap.md)。

```text
       C1       C2       C3       C4
R1     VIA1     AG00     AG01     VIA2
R2     AG02     AG03     AG04     AG05
R3     ACT06    ACT07    ACT08    ACT09
R4     VIA5     ACT10    ACT11    ACT12

右侧：A 左上旋钮 = 摇杆 X；B 右上旋钮 = 摇杆 Y
      C 下方大旋钮 = 原厂编码器
```

| KB16 位置 | Codex 事件 / 用途 | LED（从 0 开始） |
|---|---|---|
| R1C1 | VIA1，直接自定义，支持按住 | 0，无本地白光，常灭 |
| R1C4 | VIA2，自定义普通键，可按住 | 3，无本地白光，常灭 |
| R1C2 / R1C3 | AG00 / AG01 | 1 / 2 |
| R2C1 / R2C2 / R2C3 / R2C4 | AG02 / AG03 / AG04 / AG05 | 4 / 5 / 6 / 7 |
| R3C1 / R3C2 / R3C3 / R3C4 | ACT06 / ACT07 / ACT08 / ACT09 | 8 / 9 / 10 / 11 |
| R4C1 | VIA5，自定义普通键，可按住，不再强制长按切层 | 12，仅跟随第四排环境灯，无本地白光 |
| R4C2 / R4C3 / R4C4 | ACT10 / ACT11 / ACT12 | 13 / 14 / 15 |

| 旋钮 | 逆时针 | 顺时针 | 按下 |
|---|---|---|---|
| A（左上） | `JOY_LEFT`，摇杆左 | `JOY_RIGHT`，摇杆右 | VIA3，第一页自定义 |
| B（右上） | `JOY_DOWN`，摇杆下 | `JOY_UP`，摇杆上 | VIA4，第一页自定义 |
| C（下方大旋钮） | `ENC_CC` | `ENC_CW` | `ENC_SW` |

摇杆是离散方向事件模拟，不是连续二维模拟轴。ACT10/ACT11 对应原厂 MIC 的两个半键，可以在支持的宿主里合用为麦克风，也可拆分为其他动作；**没有独立的第 17 个 MIC 键**。宿主内修改 AG/ACT 的功能绑定，不会改写这张固件物理事件表，也不等于 Work Louder Input 配置。

## 小屏幕展示

以下由**当前固件测试实际输出的 128×32 帧缓冲**生成，不是实机照片。所有画面保留左右至少 6 px、上下至少 4 px 安全边距，不显示 DOIO 开机页。

**上电等待：** `VIA WAITTING`（保留指定拼写），最多 10 秒；VIA 锁定后提前结束等待页。

![上电 VIA WAITTING](docs/assets/oled-via-waiting.svg)

**普通层空闲：** L1 / L2 / L3 分别居中显示 `2` / `3` / `4`。

![普通 L1 层号 2](docs/assets/oled-via-1.svg)
![普通 L2 层号 3](docs/assets/oled-via-2.svg)
![普通 L3 层号 4](docs/assets/oled-via-3.svg)

**普通层按键时：** 临时替换层号，显示实际发出的 HID 键名/组合（如 `GUI+C`）、媒体或鼠标输出；按住保留，松开约 900 ms 回层号。显示的不是输入法文字或应用最终动作，不保存按键日志。

![实际输出 GUI+C](docs/assets/oled-via-shortcut.svg)

**进入 Codex：** 从首个可显示帧起展示终端 Logo + `CODEX` 约 1 秒；有任务灯分配时随后显示六个 AG 灯的状态估计，无任务灯分配时持续显示 Logo。任务结束并清除全部任务灯后回到 Logo，不再因 RGB 全灭而关屏。亮度 0 不等于任务释放，只有未分配或明确 `off` 才算无任务。

开发版五处自定义发出普通 HID 时，实际键名/组合/媒体/鼠标输出临时覆盖 Logo/任务页，按住保留、松开约 900 ms 恢复；宏显示最近输出。原生 AG/ACT 不产生普通键提示。见 [完整规则](docs/codex-via-keys.md)。

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
| 顶部自定义方键 | R1C1 / R1C4 | 无新增本地白光 | 不因按住而点亮 |
| 其他自定义方键 | R1C4 / R4C1 | 无新增本地白光 | R1C4 灭；R4C1 仅跟随环境灯 |

环境灯覆盖 R4C1–R4C4，不覆盖六 AG 或第三排。任务运行时环境灯也可能亮，**亮灯不等于正在录音**；蓝/海绿规则基于色类与 snake，不是宿主单独传了“运行/录音”状态。

### 优先级、扩展灯和息屏

- USB 挂起和一小时空闲休眠关闭 RGB/OLED；宿主明确关灯只屏蔽 RGB 及本地白光，不再关闭 OLED。按键和切层始终不受影响。
- 在允许显示时：ACT 上已分配的扩展任务灯 > 第四排环境 > 普通 ACT 按住白光。扩展任务 B=0 仍占灯位，`off` 才释放。
- 固件支持任务灯槽 **0–12**：0–5 对应 AG00–AG05，6–12 对应 ACT06–ACT12 的 LED；**输入仍是 ACT，不会变成 AG06–AG12**。已检查的 Cindy 构建仍只下发六路，十三路需要宿主配套，不能仅凭此固件实现十三任务端到端灯光。
- VIA 普通层按自己的 QMK RGB 配置运行，无 Codex 任务/环境覆盖；**一小时无输入或有效通信，背光与 OLED 同时关闭**。
- Codex 背光仍跟随支持灯区的聚合状态：全部 off、零亮度或非变色效果全黑时关闭 RGB；呼吸最低点不算关灯。**OLED 仅因 USB 挂起或一小时无输入且无有效通信而关闭**，不跟随任务灯全灭或亮度 0；协议没有单独的 OLED 关屏指令。
- **有效心跳与重复报文都会续时**。空闲超时后，输入或有效通信可唤醒 OLED；RGB 若仍被宿主明确关闭，则须重新下发点亮状态。灯光策略取决于当前层，不取决于 USB 身份。

完整分类阈值、参数兼容、优先级及实机验收记录：[灯光表](docs/lighting.md) · [实现边界](docs/lighting-compatibility.md)。

## 与原版 Codex Micro 的差异

| 项目 | 原版形态 / 行为 | 本项目 KB16 |
|---|---|---|
| 硬件 | 编码器 + 摇杆占据顶角，其他位置是功能键 | 16 方键 + 3 旋钮；顶角两键、左下角、A/B 按下可用 VIA，自配层切换 |
| 摇杆 | 实体摇杆 | 两个顶部旋钮模拟 X/Y 方向事件，无连续模拟位置 |
| 编码器 | 原厂编码器 | 下方大旋钮 C |
| MIC | ACT10 + ACT11 两半键 | 第四排中间两键，可由宿主合用/拆分 |
| 灯带 / 环境光 | 原厂独立环境灯区域 | 映射第四排四颗键灯；snake 变呼吸，海绿录音外观常亮 |
| 按键灯 | 原厂物理布局对应宿主灯态 | 六 AG + 七 ACT 扩展槽；Idle 常亮 5%，普通 ACT 默认灭、按住白光 |
| 屏幕 | 不依赖 KB16 的 OLED | 增加留边 Logo、六灯状态估计、普通层号及实际输出提示 |
| 普通键盘 / 改键 | 原厂产品自身配置体系 | 普通 L1–L3 + 第一页 L0 保留原生控制、开放五处；10 秒 VIA 身份协商 |
| Work Louder Input | 原厂支持的硬件配置工具 | **不实现 Input 改键、`fs.*`、Mapper / Channel 3** |
| 电量 / 无线 | 取决于原厂设备 | KB16 为 USB 有线，无真实电池遥测；协议兼容应答不等于有电池 |
| 协议完整性 | 原厂固件 | 兼容已实现的 Codex Channel 2，不是原厂固件的完整复刻 |

本项目不是官方固件，不保证所有 App 版本、原厂扩展命令、动画或外设功能完全一致。宿主侧功能绑定与灯态通信依赖支持 Codex Micro 的 App；**普通层不运行 App 也可用**。

## 流水线构建与发布

- 推送 `main` 或提交 PR：运行固件回归、发布包安全检查和 QMK 编译，产物可在 Actions 的 Artifacts 下载，不自动发布正式版本。
- 推送 `v*-qmk` 标签：构建该标签源码，验证标签与固件版本完全相同，再自动发布 Release。
- 手动运行 `Firmware`：`release_tag` 留空只构建；填写已有版本标签则按该标签重建并发布，不移动标签。已有同名附件默认拒绝覆盖，仅勾选 `replace_assets` 时替换。
- 固定 QMK commit、QMK CLI 1.2.0、Arm GNU Toolchain 15.3.Rel2，并用固定 SHA-256 校验工具链下载；Actions 本身固定完整提交。构建 job 只读，只有通过验证的发布 job 有写权限，不需要额外 PAT。

例如重新生成本次 Release（不是上传本地文件）：

```bash
gh workflow run firmware.yml --ref main \
  -f release_tag=v0.4.8-qmk -f replace_assets=true
```

流水线检查 DFU CRC、应用向量/地址、BIN 与 HEX 一致性、版本/USB 描述符、VIA 定义，以及附件确实来自当前仓库/本次运行，再发布 BIN、HEX、VIA JSON、`BUILD-INFO.json` 和 `SHA256SUMS`。构建失败不会进入发布 job，也不会刷键盘。

## 自行构建与测试

使用**单独的 QMK 源码树**，基线固定为 `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`。需要 QMK CLI、带 newlib 的 Arm GNU Toolchain；离线测试还需 Python 3 和 Clang。不要在有其他修改的 QMK 工作区上直接覆盖 core patch。

```bash
# 先准备好上述固定版本的 QMK 及其 submodules、工具链
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
QMK_HOME="$QMK_HOME" python3 firmware/keymaps/codex_micro_via_v1/tools/run_tests.py
```

产物为 QMK 根目录的 `doio_kb16_rev2_codex_micro_via_v1.bin`，供本地开发验证，不能作为正式 Release 附件。补丁脚本会检查基线，并复制 keymap、USB/VIA/ChibiOS/WS2812 补丁；不要只复制 `keymap.c`。构建、恢复、SHA-256 和验证边界见 [开发](docs/development.md) / [刷写](docs/usage.md)。

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
