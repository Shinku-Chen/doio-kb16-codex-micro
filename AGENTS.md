# AGENTS.md

给 AI agent（Claude Code / Codex / Cursor 等）读的仓库说明。

## 项目概述

DOIO KB16-01 Rev2（APM32F103CBT6，128 KB Flash）的 Codex Micro 兼容固件与配套工具。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用，并提供键位/灯光配置、OLED 图标生成和刷写文档。

上游参考：`hcyniubi/doio-kb16-codex-micro`。本仓库是私有工作副本，后续按自己的需求修改。

## 目录结构

本仓库只负责生成固件，不包含 Mapper 或预编译产物；PoC 骨架位于 `firmware/keymaps/codex_micro_configurable_v1_3/poc_*`。目录如下：

```
firmware/        QMK 固件与 keymap（codex_micro_configurable_v1_3）
docs/            使用、硬件、协议、OLED、开发、FAQ、PoC 与额度文档
firmware/qmk-core-patches/  USB/VIA/OLED 等 QMK core patch 与设计稿
```

文档入口：`docs/README.md`；需求基线：`docs/requirements.md`。

## 构建与验证

> 源码已导入；需要先完成 QMK setup，并按 `firmware/qmk-core-patches/` 应用 core patch。仓库不提交 `.bin` / `.hex` / `.uf2` 等构建产物。

```bash
# QMK 固件（需先安装 QMK 与 ARM 工具链）
qmk compile -kb doio/kb16/rev2 -km codex_micro_configurable_v1_3
qmk lint -kb doio/kb16/rev2 -km codex_micro_configurable_v1_3

# 静态校验（上游提供）
python3 firmware/keymaps/codex_micro_configurable_v1_3/tools/validate_keymap.py
```

改动后至少跑构建与静态校验；涉及协议/键位/灯光时补跑对应测试。刷写前必须确认硬件为 KB16-01 Rev2、Bootloader 为 STM32duino/Maple，并核对固件 SHA-256。

## PoC 开关

```bash
# VIA/Codex 顺序模式 + OLED 仪表盘
qmk compile -kb doio/kb16/rev2 -km codex_micro_configurable_v1_3 \
  POC_MODE_SWITCH=yes POC_OLED_STATUS=yes
```

- `POC_MODE_SWITCH=yes`：`poc_mode.c` / `poc_usb.c` / `poc_via.c`
- `POC_OLED_STATUS=yes`：`poc_oled.c`（层号 + 任务状态）
- `POC_OLED_QUOTA=yes`：`poc_quota.c`（可选额度，默认关闭；需要 `POC_OLED_STATUS=yes` 和 host daemon）
- 还需要 `VIA_ENABLE=yes`、`DYNAMIC_KEYMAP_ENABLE=yes`，以及 `firmware/qmk-core-patches/POC-usb-switch.md`、`POC-via-layers.md` 描述的 core patch。
- 设计说明：`docs/poc.md`；额度：`docs/quota.md`。

## 硬件与协议边界

- 目标硬件：DOIO KB16-01 Rev2，APM32F103CBT6，128 KB Flash，STM32duino/Maple Bootloader。
- 原厂 VIA 身份：VID `0xD010` / PID `0x1601`。
- Codex Micro 模式：VID `0x303A` / PID `0x8360`，Raw HID usage `0xFF00:0x0001`，Report ID `6`，64 字节报告。
- 4 个固定层：Codex、Number、Navigation、System；Codex 层 16 个原生控制必须各出现一次。
- OLED：128×32 单色，一帧 512 字节；默认 4 个 24×24 层图标 + 标签。
- 不要在未确认 Rev2 的设备上刷写；不要修改 Bootloader 或读保护设置。

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
