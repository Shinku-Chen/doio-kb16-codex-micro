# Changelog

## Unreleased

- 初始化私有仓库与项目文档：新增 `AGENTS.md`、`CHANGELOG.md`。
- 新增项目文档：`docs/` 下的 README、硬件、使用、Codex Micro 对比、协议、OLED、开发与 FAQ。

- 导入上游固件、Mapper、QMK core patches 与上游文档。
- 新增 PoC 骨架：模式标志、30s/60s 切换、USB 描述符选择 stub、VIA 命令检测、OLED 层号/任务状态（额度默认关闭）。
- 新增 `docs/poc.md`、`docs/quota.md` 以及 USB/VIA core patch 设计稿。

- 额度显示默认关闭：默认 PoC 只显示层号和任务状态，不启用任何需要额外 host 程序的功能；`POC_OLED_QUOTA` 作为可选开关保留。

- 移除预编译固件二进制与 `firmware/SHA256SUMS.txt`；新增 `.gitignore`，固件由本地构建、不再入库。

- 移除 Mapper；仓库只保留固件源码/构建/文档，不再包含 Windows 配置工具。
