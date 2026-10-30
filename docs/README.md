# DOIO KB16 Codex Micro 文档

本仓库是 DOIO KB16-01 Rev2 的 Codex Micro 兼容固件与工具的工作副本。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用。

> 当前开发版 `0.6.0-qmk`：Codex 位于第一页 L0，普通 VIA 为 L1–L3；五处直接自定义，取消固定长按退出，自己设置 MO/TO 切层。旧配置与层动作目标一次迁移，先备份。73 组离线检查、13 项打包测试及本地编译通过，本版尚未刷入，设备仍是 0.5.0；刷写使用对应提交的 main 流水线 Artifact。详见 [五处改键](codex-via-keys.md)、[开发](development.md)。正式镜像由流水线生成，刷机需拆外壳按内部 RESET，见 [使用](usage.md)。展示见 [中文 README](../README.md) / [English README](../README.en.md)。

## 快速开始

1. 确认硬件：DOIO KB16-01 **Rev2**（APM32F103CBT6），不是 Rev1 或无线版。
2. 正式固件从 [Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases) 下载流水线附件，核对来源及 SHA-256 后刷入；本地构建只用于开发（见 `development.md`、`usage.md`）。
3. 安装并登录 ChatGPT/Codex 桌面版，打开 Settings → Codex Micro 配置 AG 键、摇杆、旋钮、麦克风。
4. 开机十秒内让 VIA 读取，第一页面 L0 修改五处自定义，第二至第四页修改普通 L1–L3。建议 L0 配 MO(1) 按住普通 L1，松开恢复；普通层配 TO(0) 返回 Codex。本仓库不包含 Mapper。

## 文档索引

- `codex-via-keys.md` — 0.6.0 Codex L0 五处直接 VIA 自定义、自配 MO/TO、升级迁移和 OLED 提示
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
