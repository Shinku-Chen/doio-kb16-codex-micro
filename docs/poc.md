# PoC：VIA / Codex 顺序模式

## 目标

插电后先进入 VIA 模式，30 秒内如果收到 VIA 命令就一直保持 VIA；否则 30 秒后切到 Codex Micro。VIA 连接后不会自动切回 Codex；改完/刷完后断电重插。

分层职责：

- 层 0：纯 Codex 原生控制，固定
- 层 1-3：Number / Navigation / System，VIA 在线改键

OLED 显示：当前层号、六个任务状态。额度显示默认关闭（需要 host daemon，暂不做）。

## 状态机

```text
上电
  └─ VIA 模式（0xD010:0x1601，FF60:0061）
       ├─ 30s 内收到 VIA 命令 → via_seen=true，保持 VIA
       ├─ 30s 内没有 VIA 命令 → 切换 Codex
       └─ via_seen → 一直保持；断电重插后重新计时

切换 = 写 .noinit 标志 → reset_keyboard()
  └─ 启动时读到标志 → Codex 模式（0x303A:0x8360，FF00:0001）
```

## 代码位置

`firmware/keymaps/codex_micro_configurable_v1_3/`：

- `poc_config.h` — 30s 窗口、层偏移、magic
- `poc_mode.c/h` — 模式标志、计时、强制切换
- `poc_usb.c/h` — USB 描述符选择 stub
- `poc_via.c/h` — VIA 命令检测、层数量/偏移
- `poc_quota.c/h` — 可选额度 JSON 解析，默认不编译
- `poc_oled.c/h` — OLED 仪表盘
- `keymap.c` — `keyboard_init_user` / `matrix_scan_user` / `oled_task_user` 挂钩
- `rules.mk` — `POC_MODE_SWITCH` / `POC_OLED_STATUS` / `POC_OLED_QUOTA`

## 启用

```bash
qmk compile -kb doio/kb16/rev2 -km codex_micro_configurable_v1_3 \
  POC_MODE_SWITCH=yes POC_OLED_STATUS=yes
```

还需要：

- `VIA_ENABLE=yes`、`DYNAMIC_KEYMAP_ENABLE=yes`（PoC 里尚未打开）
- 应用 `firmware/qmk-core-patches/POC-usb-switch.md` 和 `POC-via-layers.md` 描述的 core patch
- 使用原厂 KB16-01 VIA definition（VIA 模式身份 `0xD010:0x1601`）

默认不启用任何需要额外 host 程序的功能。额度显示是可选开关，默认关闭。

## 已实现 / 未实现

**已实现**

- 模式标志、30s 计时、VIA 命令检测骨架
- Codex raw HID 处理拆成 `codex_micro_raw_hid_receive`，由 `via_command_kb` 路由；避免和 QMK `via.c` 的 `raw_hid_receive` 符号冲突
- `.noinit` magic + `reset_keyboard()` 切换
- OLED 层号、六个任务状态（默认，不需要额外 host 程序）
- 可选额度解析：`POC_OLED_QUOTA=yes` 时才编译 `poc_quota.c` 并接受 `v.oai.quota`

OLED 的任务字母来自 `v.oai.thstatus` 的颜色/effect，不是任务标题；host 没发状态时显示 `.`。额度需要 host daemon，键盘本身不联网；默认不启用。

**未实现**

- USB 描述符真正切换（`poc_usb.c` 只是 stub，需要 core patch）
- 打开 `VIA_ENABLE` / `DYNAMIC_KEYMAP_ENABLE`；描述符、层 offset 和 EEPROM 分区就绪后再开
- VIA 的 layer offset 真正接入 `dynamic_keymap_*`（需要 core patch）
- EEPROM 分区（Codex A/B 配置 vs QMK dynamic keymap）
- host 侧额度 daemon（默认不做；只有 `POC_OLED_QUOTA=yes` 时才需要，见 `quota.md`）
- VIA 模式的按键/编码器写入验证

## 风险

- `.noinit` section 是否被 STM32duino bootloader 保留要实测；不保留就改用 RTC backup 寄存器。
- 30 秒窗口对首次打开 VIA 偏紧；可改 60 秒或按住键插电强制 VIA。
- 切换会重新枚举 USB；ChatGPT 是否自动重新识别要实测。
- VIA 写入过程中拔电可能丢写；确保写入完成后再断电。
- VIA 层偏移、EEPROM 分区和 encoder 支持都需要实机验证。
