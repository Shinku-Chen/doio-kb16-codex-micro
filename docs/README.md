# DOIO KB16 Codex Micro 文档

本仓库是 DOIO KB16-01 Rev2 的 Codex Micro 兼容固件与工具的工作副本。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用。

> 当前源码 `0.4.8-qmk`：上电 VIA 窗口 10 秒、Codex 左下角长按 1 秒回 L0；60 项离线检查及 QMK 编译通过，本版未刷机。上一版 `0.4.7-qmk` 已刷并获用户正常反馈，不据此推断新版时长或一小时休眠已实测。完整展示见 [中文 README](../README.md) / [English README](../README.en.md)。

## 快速开始

1. 确认硬件：DOIO KB16-01 **Rev2**（APM32F103CBT6），不是 Rev1 或无线版。
2. 从 [Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases) 下载固件或本地构建，核对 SHA-256 后刷入（见 `development.md`、`usage.md`）。
3. 安装并登录 ChatGPT/Codex 桌面版，打开 Settings → Codex Micro 配置 AG 键、摇杆、旋钮、麦克风。
4. 用 VIA 修改 L0–L2；第四页为可编辑保存的 Codex 入口占位，不会改变固定 Codex 布局。本仓库不包含 Mapper。

## 文档索引

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
