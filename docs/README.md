# DOIO KB16 Codex Micro 文档

本仓库是 DOIO KB16-01 Rev2 的 Codex Micro 兼容固件与工具的工作副本。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用。

> 当前状态：仓库刚完成文档初始化，固件/工具源码尚未导入；构建和刷写命令在源码导入后生效。

## 快速开始

1. 确认硬件：DOIO KB16-01 **Rev2**（APM32F103CBT6），不是 Rev1 或无线版。
2. 刷入 Codex Micro 固件（见 `usage.md`）。
3. 安装并登录 ChatGPT/Codex 桌面版，打开 Settings → Codex Micro 配置 AG 键、摇杆、旋钮、麦克风。
4. 需要改普通层键位/灯光时，用 VIA（PoC）或外部上游 Mapper；本仓库不含 Mapper。

## 文档索引

- `requirements.md` — 需求基线、功能/非功能需求、验收标准
- `full-implementation.md` — 完整实现 Codex Micro 固件的路线与可行性
- `input-compatibility.md` — Work Louder Input 兼容方案与 fs/keymap PoC 路线
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
