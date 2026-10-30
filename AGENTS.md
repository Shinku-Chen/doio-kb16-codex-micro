# AGENTS.md

给 AI agent（Claude Code / Codex / Cursor 等）读的仓库说明。

## 项目概述

DOIO KB16-01 Rev2（APM32F103CBT6，128 KB Flash）的 Codex Micro 兼容固件与配套工具。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用，并提供键位/灯光配置、OLED 图标生成和刷写文档。

上游参考：`hcyniubi/doio-kb16-codex-micro`。本仓库是私有工作副本，后续按自己的需求修改。

## 目录结构

本仓库只负责生成固件，不包含 Mapper 或预编译产物。目录如下：

```text
firmware/                              QMK 源码：keymap、VIA 定义、core patch、构建脚本
firmware/keymaps/codex_micro_via_v1/   当前唯一实现（QMK + VIA 双模式）
firmware/via/kb16_codex_via.json       VIA 定义
firmware/qmk-core-patches/             需要覆盖到 QMK 树的完整文件
firmware/keymaps/codex_micro_configurable_v1_3/  旧上游 v1.3 keymap（历史参考）
kb16-fw/                               Rust/embassy 尝试（已放弃，历史参考）
docs/                                  使用、硬件、协议、OLED、开发、FAQ、双模式设计
```

文档入口：`docs/README.md`；需求基线：`docs/requirements.md`；双模式设计：`docs/via-dual-mode.md`。

## 构建与验证

> 当前主实现是 `firmware/keymaps/codex_micro_via_v1`（QMK + VIA 双模式）。`kb16-fw/` Rust 尝试已放弃。仓库不提交 `.bin` / `.hex` / `.uf2` 等构建产物。

2026-10-30 最新实机 `0.4.7-qmk` 已刷入并通过 RPC 核验，用户随后反馈“目前没问题”。当前源码 `0.4.8-qmk` 将 VIA 冷启动窗口从 30 秒缩为 10 秒、Codex 退出长按从 5 秒缩为 1 秒；60 项离线检查及 QMK 编译通过，本版未刷机。固件通过 GitHub Release 分发，Git 不提交 BIN/HEX；双语 README 展示当前键位/灯效及从测试帧生成的 OLED 画面，不能称为实机照片。完整哈希与验收边界见 `docs/development.md`。

```bash
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```

`qmk lint` 目前只会因为 keymap 文件缺少 license header 报错（上游 keymap 同样没有 header，暂不补）。旧 keymap `codex_micro_configurable_v1_3` 不再维护，新需求一律改 `codex_micro_via_v1`。

## 硬件与协议边界

- 目标硬件：DOIO KB16-01 Rev2，APM32F103CBT6，128 KB Flash，STM32duino/Maple Bootloader。
- 原厂 VIA 身份：VID `0xD010` / PID `0x1601`。
- Codex Micro 模式：VID `0x303A` / PID `0x8360`，Raw HID usage `0xFF00:0x0001`，Report ID `6`，64 字节报告。
- 当前定稿为四层（`0.3.0-qmk`）：Layer 0–2 为普通 VIA 层，Layer 3 固定运行 Codex；VIA 显示 0–3 四页且均可读写，但第四页数据只存储、不执行，不能用它修改实际 Codex 按键或编码器。`DYNAMIC_KEYMAP_LAYER_COUNT=4`，`TO(3)` 进入 Codex，L3 左下角长按 1 秒回 L0，不在普通层强占按键。切层不改变 USB 身份；上电 10 秒内收到有效 VIA 命令锁定 VIA 身份，否则复位成 Codex 身份并默认 L3。模式标志在 `BKP->DR9`，`BKP->DR10` 属于 STM32duino bootloader。
- 四层升级保留旧键位/编码器/宏的 EEPROM 起址，用未使用空间保持地址兼容，并非保留可进入的第五层。一次迁移 L0–L2 按键/编码器中指向旧 L4 的层动作到 L3；VIA 第四页不重置、后续编辑不反复覆盖。刷前仍应导出 VIA 配置；旧五层说明以当前四层模型为准。
- 灯光以宿主 c/b/e/s（原始 RGB、亮度、效果、速度）为准，不维护 GPT/Cindy 等 App 专属色值白名单，也不把收到的颜色替换为预设色。通用 HSV 色类识别、宿主参数和本次输出微调分开；识别只返回色类，微调只操作输出副本，不回写宿主原值。用户在 2026-10-30 改为 AG 任意颜色 snake→0–100%×宿主亮度原色呼吸（`0.4.4-qmk`），跟随宿主速度，0 暂停当前相位，不再固定 5 Hz；保留白/灰 Idle 常亮 5%×宿主亮度，呼吸及 snake 转呼吸均不叠加 Idle 降亮。环境灯仍覆盖第四排 LED 12–15：蓝色系 snake→0–10%×宿主亮度同步呼吸，海绿色系 snake→原亮度常亮，其他 snake→0–100% 呼吸；速度跟随宿主，0 暂停，显式 breath/solid/shallowBreath 不受这些环境例外影响。不改色、不覆盖 AG；环境优先于这排 ACT 按压，切层键按住白光优先、1 秒退出不变。原色和本地按压反馈是不同来源，不能用本地白光覆盖宿主颜色。完整阈值、覆盖关系和验收边界见 `docs/lighting.md`。
- OLED 最新规则（`0.4.0-qmk`）：128×32 单色，左右至少 6 px、上下至少 4 px 留黑。切到 L3 先显示旧终端图标+CODEX 约 1 秒，再显示已有六个 AG 的任务状态；无任务标题/额度。L0–L2 居中显示大号 1/2/3；USB 输出报告成功入队后临时显示实际键名/组合/媒体/鼠标输出，按住保留、松开约 900 ms 恢复数字，长提示最多两行。取 6KRO/NKRO 最终报告，不靠旧键位猜测，不把输入法文字或应用动作当成可见内容，不存输入日志、不加 host daemon。`POC_OLED_INPUT` 仅在当前 keymap 启用，观察函数只更新 RAM，绘制在 OLED task；通用 REQUESTED/NO LINK 弹窗仍不覆盖任务页。旧“只显示0–3层号”方案已被本次用户确认替代，详见 `docs/oled.md`。
- OLED 的 IDLE/RUN/WAIT/NEW/ERR 由通用色类推断，不是宿主显式业务状态：白/灰→IDLE、蓝→RUN、橙/黄→WAIT、绿/海绿→NEW、红→ERR，青/紫/粉等含义不明时 OTHER。协议只有灯光参数，不能声称能识别任意主题、品牌预览、真正的录音或任务语义；例如红色预览也会推断为 ERR。海绿色环境常亮和蓝色环境降亮同样是色类外观规则，不是获得了独立录音/运行标志。历史 `0.4.2-qmk` 的精确色别名方案已被用户否定，不要恢复白名单或声称“未知 RGB 必须 OTHER”；相近颜色由同一色类处理，RGB 本身保持不变。
- 当前休眠以 `0.4.6-qmk` 的 `poc_power.c` 为唯一策略：L0–L2 为 VIA 普通层，按键/旋钮及有效宿主通信均续时，连续一小时无活动同时关闭 RGB/OLED；L3 跟随所有受支持任务灯/环境灯的聚合下发状态，全部 off、零亮度或非变色效果全黑时同步关闭，任一组恢复则点亮，另有一小时静默兜底。用户在 2026-10-30 明确纠正：有效心跳和重复报文也必须续时，不能只在灯态变化时续时；sys.version/device.status、完整重复灯态 RPC 和有效 VIA 命令均算通信活动，坏帧/无 method 不算。宿主明确关灯和 USB 挂起优先，心跳只续时，不能推翻关灯；宿主关灯时本地 ACT/切层白光也被统一屏蔽，但按键输入和一秒退出不受影响。未收到灯态前保留 Logo/任务页，空闲一小时仍会关闭。呼吸低点不代表关灯，黑色基色的 rainbow/gradient 仍可发光。原生 `RGB_MATRIX_TIMEOUT/OLED_TIMEOUT` 均为 0 只是关闭独立计时器，真正一小时门限是 `POC_IDLE_SLEEP_MS=3600000UL`，禁止据此恢复“无空闲关灯”旧说明。息屏不改灯态/EEPROM，因超时关闭后有效通信或输入可恢复；若宿主明确关灯仍须宿主重新点亮。USB 挂起两者同灭，恢复时仍受同一策略约束，OLED 帧未变也能重新点亮。
- `0.4.5-qmk` 固件灯槽支持 0–12：0–5 是六个 AG 灯，6–12 固定对应 ACT06–ACT12 物理灯，输入仍上报原生 AG/ACT；这是 KB16 扩展，不代表原厂协议或现有 Cindy 已支持十三路。扩展任务启用时，其灯态（含 B=0）优先于本地 ACT 白光及第四排环境；e=off 释放灯位，恢复原优先级。空位和切层键不作为额外任务灯，OLED 仍显示原有六格，不能简单扩大循环导致越界。RPC 重组容量扩为 2048，覆盖十三槽完整 SDK 帧；局部更新省略 e 必须保持原效果，不能把 off 自动恢复为 solid。Cindy 已安装构建 `b3dd8436…` 的打包代码仍固定下发六路，虽然软件允许把 ACT 配为任务键；十三路实用效果需宿主按物理键投影并下发扩展灯槽，未经用户要求不修改/替换 Cindy。必要性评估和映射见 `docs/lighting-compatibility.md`。
- 不要在未确认 Rev2 的设备上刷写；不要修改 Bootloader 或读保护设置。
- `0.4.7-qmk` 按用户要求取消 QMK 板级 DOIO 五秒启动页：当前 keymap 定义 `OLED_LOGO_TIMEOUT=0`，不能只改用户 OLED renderer 而遗漏板级抢占。冷启动 VIA 等待窗口居中显示用户指定原文 `VIA WAITTING`，仍保留安全边距；收到有效 VIA 命令锁定身份后恢复层号/按键页，无命令到三十秒则复位进 Codex 的 Logo+CODEX/任务页。等待状态由 `poc_mode_via_waiting()` 提供，不另外重计一个 OLED 三十秒。RET/刷写后的复位可能沿用 BKP 模式而直接回 Codex；需要真正断电上电才能验收冷启动等待页。启动文字/移除 Logo 与实机已显示是不同验收项，必须分别记录。

## 代码约定

- QMK C 风格：4 空格缩进，函数/变量 `snake_case`，宏 `UPPER_SNAKE_CASE`。
- 固件内避免动态内存分配；改动前评估 Flash/RAM 预算（128 KB Flash）。
- 对不自解释的函数、结构体字段、协议字段和请求参数补简洁注释，说明职责、语义和使用边界。
- 修改协议、键位、灯光或 OLED 渲染时，同步新增/更新对应测试（`tools/` 下的 C/Python 测试）。
- 本项目不涉及 Redis；如未来引入缓存，默认设置 TTL，不允许长期缓存。
- 注释与文档默认中文，技术术语、类型名、API 保留英文。

## 提交规范

- commit 标题：`type(scope): 简述`，`type` ∈ `feat/fix/docs/refactor/perf/test/chore/build/ci`；简述用祈使句、≤50 字符、结尾不加句号、默认中文。
- 一个 commit 只做一件事，message 描述最终 diff，不叙述调试过程。
- 个人仓库可直接推 `main`；协作或需要 review 时走分支/PR。
- 任何实际文件变更都同步记录到 `CHANGELOG.md`。
- 需求沟通形成的最终需求、项目决策、踩坑和运行方式，必须回写到本文件或 `docs/`，不要只留在会话记忆里。

## 其他约定

- 上游代码/文档沿用其 LICENSE；引入第三方代码时保留版权与许可声明。
- Mapper 仅支持 Windows x64；macOS 侧如需改键/改灯，使用自定义工具或 Windows 虚拟机。
- 刷写/回退流程必须写进 `docs/`，包含 SHA-256、Bootloader 识别和失败恢复步骤。
