# 开发与构建

## 当前状态

仓库刚完成文档初始化，固件/工具源码尚未导入。以下流程基于上游 `hcyniubi/doio-kb16-codex-micro`，源码导入后按实际路径调整。

## 计划目录

```
firmware/        QMK 固件与 keymap
mapper/          Windows WPF 配置工具
docs/            本文档
tools/           OLED 图标生成、校验、刷写脚本
```

## 环境

- QMK（`qmk setup`）
- ARM 工具链（QMK 安装脚本会处理）
- Python 3 + Pillow（生成 OLED 图标）
- `dfu-util`（刷写）

## 导入上游源码

1. 克隆上游 `hcyniubi/doio-kb16-codex-micro`。
2. 把 `firmware/keymaps/codex_micro_configurable_v1_3/` 复制到 QMK 的 `keyboards/doio/kb16/rev2/keymaps/`。
3. 按上游 `firmware/qmk-core-patches/` 应用 4 个 core patch（USB 描述符、Raw HID、DMA 等）。
4. 保留上游 `LICENSE` 和版权声明。

## 构建

```bash
qmk compile -kb doio/kb16/rev2 -km codex_micro_configurable_v1_3
qmk lint -kb doio/kb16/rev2 -km codex_micro_configurable_v1_3
```

生成 `doio_kb16_rev2_codex_micro_configurable_v1_3.bin`。

## 静态校验

```bash
python3 firmware/keymaps/codex_micro_configurable_v1_3/tools/validate_keymap.py
```

上游还提供 C 测试（`tools/protocol_test.c`、`tools/config_test.c`），按上游说明编译运行。

## 刷写

```bash
dfu-util -a 2 -d 1EAF:0003 -D doio_kb16_rev2_codex_micro_configurable_v1_3.bin
```

进入 Bootloader：拔 USB，按住左上角键，再插 USB。

## 硬件验收

刷入后至少验证：

- ChatGPT 能识别为 Codex Micro
- 6 个 AG 键事件正常
- ACT06-09/12、Mic 正常
- 左旋钮 `ENC_CC/CW/SW` 正常
- 四方向摇杆事件正常
- 6 个任务灯颜色/呼吸正常
- OLED 层图标和弹窗正常
- 能进入 Bootloader 并刷回官方 VIA 固件

## 提交

- commit：`type(scope): 简述`，中文，≤50 字符，结尾不加句号。
- 一个 commit 只做一件事。
- 同步更新 `CHANGELOG.md` 和相关文档。
- 需求/决策/踩坑回写到 `AGENTS.md` 或 `docs/`。
