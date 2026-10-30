# DOIO KB16 Codex Micro 文档

本仓库是 DOIO KB16-01 Rev2 的 Codex Micro 兼容固件与工具的工作副本。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用。

> 开发源码 `0.5.0-qmk` 开放 L3 五处 VIA 自定义，固定一秒退出移到左上方键，左下角直接执行第四页配置，OLED 显示实际输出；新功能未发布/刷入，见 [五处改键](codex-via-keys.md)。实机仍为流水线 0.4.9，已核验版本与入层 Logo；一小时息屏/按键唤醒未实测。刷机需拆外壳按 PCB 内部 RESET，详见 [使用与刷写](usage.md) / [开发记录](development.md)。完整展示见 [中文 README](../README.md) / [English README](../README.en.md)。

## 快速开始

1. 确认硬件：DOIO KB16-01 **Rev2**（APM32F103CBT6），不是 Rev1 或无线版。
2. 正式固件从 [Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases) 下载流水线附件，核对来源及 SHA-256 后刷入；本地构建只用于开发（见 `development.md`、`usage.md`）。
3. 安装并登录 ChatGPT/Codex 桌面版，打开 Settings → Codex Micro 配置 AG 键、摇杆、旋钮、麦克风。
4. 用 VIA 修改 L0–L2；开发版 0.5.0 允许第四页指定五处执行 VIA 配置，其余原生 Codex 布局仍固定。已发布 0.4.9 的第四页仍只作占位，本仓库不包含 Mapper。

## 文档索引

- `codex-via-keys.md` — 0.5.0 开发版五处 Codex 层 VIA 自定义、长短按、升级迁移和 OLED 提示
- `firmware-audit-2026-10-30.md` — 当前固件跨仓库检查、离线复现、构建结果与通信诊断修正
- `lighting.md` — GPT 原色、Idle 常亮 5%、环境四灯同步呼吸、录音海绿常亮、AG 灯效和完整对照表
- `via-dual-mode.md` — QMK + VIA 双模式：USB 身份、10 秒窗口、层模型、构建
- `requirements.md` — 需求基线、功能/非功能需求、验收标准
- `full-implementation.md` — 完整实现 Codex Micro 固件的路线与可行性
- `input-compatibility.md` — Work Louder Input 兼容方案与 fs/keymap PoC 路线
- `openmicrokbd.md` — OpenMicroKbd 参考实现分析（Codex + Input 全协议）
- `keymap.md` — Codex Micro ↔ KB16 键位对应图
- `hardware.md` — 硬件版本、USB 身份、Bootloader、刷写安全
- `usage.md` — 刷写、ChatGPT 设置、改键、恢复
- `codex-micro.md` — 官方 Codex Micro 对比、原生控制、摇杆/触摸/层
- `protocol.md` — USB/HID 通道、任务灯、摇杆事件、配置协议
- `oled.md` — 128×32 OLED 自定义图案
- `development.md` — 构建、刷写、测试、贡献
- `faq.md` — 常见问题
- `poc.md` — VIA/Codex 顺序模式 PoC 骨架
- `quota.md` — Codex 额度方案（默认关闭，需要 host daemon）
- `upstream/` — 上游历史文档（可能提到 Mapper/预编译固件，这些不属于本仓库）

## 安全提示

- 只刷 KB16-01 Rev2；Rev1（ATmega32U4）刷 STM32 固件会变砖。
- 刷前备份或保留官方 VIA 3.0 固件；核对 SHA-256。
- 不要在未确认 Bootloader 的情况下刷写。
