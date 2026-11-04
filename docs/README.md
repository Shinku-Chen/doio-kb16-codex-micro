# DOIO KB16 Codex Micro 文档

本仓库是 DOIO KB16-01 Rev2 的 Codex Micro 兼容固件与工具的工作副本。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用。

> 当前版本 `0.7.1-qmk`：L0 Codex 全部按键/旋钮 VIA 优先，KC_NO/KC_TRNS 回退原生；L1–L3 普通 VIA，自配 MO/TO，无固定长按。Codex L0 全部 16 个方键逐键按住白光；Codex RGB/OLED 不再一小时空闲关闭，仅普通 VIA L1–L3 保留。80 组固件检查、13 项打包测试及本地编译通过，正式镜像由标签流水线生成。2026-11-04 用户确认“按下白光”通过（基于 `db2cdf1` 同逻辑开发镜像）；正式 0.7.1 未刷入，普通层灯光、真实长时间无空闲关闭及 Flash 读回未验收。详见 [混合层](codex-via-keys.md)、[开发](development.md)。刷机需拆外壳按内部 RESET，见 [使用](usage.md)；展示见 [中文 README](../README.md) / [English README](../README.en.md)。

## 快速开始

`0.7.1-qmk` 在 `0.7.0-qmk` 的 L0 VIA 优先/空键回退基础上，加入 Codex L0 逐键按住白光和取消 Codex 一小时空闲关闭；普通 VIA L1–L3 仍保留一小时计时。现有配置保留，0.7.0→0.7.1 不新增迁移；升级前备份检查。灯光与普通层验收边界见 [灯光规则](lighting.md) 和 [刷写记录](development.md#2026-11-04功能分支在-windows-刷写)。

1. 确认硬件：DOIO KB16-01 **Rev2**（APM32F103CBT6），不是 Rev1 或无线版。
2. 正式固件从 [Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases) 下载流水线附件，核对来源及 SHA-256 后刷入；本地构建只用于开发（见 `development.md`、`usage.md`）。
3. 安装并登录 ChatGPT/Codex 桌面版，打开 Settings → Codex Micro 配置 AG 键、摇杆、旋钮、麦克风。
4. 开机十秒内让 VIA 读取，第一页 L0 修改任意按键/旋钮，清空恢复原生默认；第二至第四页修改普通 L1–L3。建议 L0 配 MO(1) 按住普通 L1，松开恢复；普通层配 TO(0) 返回 Codex。本仓库不包含 Mapper。

## 文档索引

- `codex-via-keys.md` — 0.7.1 L0 全部 VIA 覆盖、空键回退、逐键白光、Codex 无空闲关闭、自配 MO/TO 和 OLED 提示
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
