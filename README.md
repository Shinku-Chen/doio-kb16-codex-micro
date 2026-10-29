# DOIO KB16 Codex Micro

DOIO KB16-01 Rev2 的 Codex Micro 兼容固件生成仓库。目标是把 16 键 + 3 旋钮 + 128×32 OLED 的 KB16 作为 Codex Micro 控制器使用；仓库只保留固件源码/构建/文档，不包含 Mapper 或预编译固件。

- 文档入口：[`docs/README.md`](docs/README.md)
- 硬件要求：DOIO KB16-01 **Rev2**（APM32F103CBT6，128 KB Flash）
- 当前状态：源码已导入；PoC 骨架已添加；仓库只保留源码，固件由本地构建

> 仅支持 KB16-01 Rev2。Rev1（ATmega32U4）不要刷本项目的 STM32 固件。
>
> 本仓库不存放预编译 `.bin` / `.hex` / `.uf2`；构建产物由本地 `qmk compile` 生成，已被 `.gitignore` 忽略。

## 文档

- [硬件与兼容性](docs/hardware.md)
- [使用与刷写](docs/usage.md)
- [官方 Codex Micro 对比](docs/codex-micro.md)
- [协议与数据流](docs/protocol.md)
- [OLED 自定义](docs/oled.md)
- [开发与构建](docs/development.md)
- [FAQ](docs/faq.md)
- [PoC：VIA/Codex 顺序模式](docs/poc.md)
- [Codex 额度方案（默认关闭）](docs/quota.md)
