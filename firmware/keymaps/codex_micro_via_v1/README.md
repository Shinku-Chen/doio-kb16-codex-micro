# codex_micro_via_v1

QMK + VIA 双模式 keymap，目标硬件 DOIO KB16-01 Rev2（STM32F103xB，128 KB Flash）。

开发版 `0.6.0-qmk`：Codex 改到 L0，L1–L3 为普通 VIA。五处直接自定义、取消固定长按退出/短按重放/专用退出白光，其他原生控制及 USB、灯效、休眠不变。旧配置页序/层动作自动迁移，0.5.0 的五处不再清空；本版未刷入，按用户授权在验证后提交推送，正式固件必须由流水线生成。详见仓库 `docs/codex-via-keys.md`。

已发布/实机基线 `0.4.9-qmk`：OLED 不再跟随 Codex RGB 全灭关屏，无任务灯分配时保留 Logo，入层从首个可见帧展示完整约 1 秒。保留一小时静默/USB 挂起息屏、有效心跳及重复报文续时；RGB、四层、EEPROM、10 秒启动和当时左下角一秒退出不变。63 项固件离线检查通过，流水线版本已刷入，版本/切层及用户入层 Logo 目视确认通过；一小时息屏/按键唤醒按用户选择未实测。完整双语指南及展示见仓库根 README。

## 模式

- 上电 10 秒内收到 VIA raw HID 命令 → 保持 VIA 身份 `D010:1601`，Layer 0-3 可改，直到断电。
- 10 秒内没有 VIA 命令 → `soft_reset_keyboard()` → Codex 身份 `303A:8360`。
- 模式标志存在 `BKP->DR9`（`BKP->DR10` 属于 STM32duino bootloader）。

## 层

| 层 | 来源 |
|---|---|
| 0 | 原生 Codex 固定，第一页五处直接读取 VIA |
| 1-3 | 普通 QMK 动态键位/encoder map，由 VIA 改键 |

`DYNAMIC_KEYMAP_LAYER_COUNT=4`、`VIA_VISIBLE_LAYER_COUNT=4`、`POC_CODEX_LAYER=0`，没有可进入的第五层；旧编码器/宏存储地址保留，不是第五个运行层。

## Layer 0

```text
VIA1  AG00  AG01  VIA2    (直接 VIA 自定义)
AG02  AG03  AG04  AG05
ACT06 ACT07 ACT08 ACT09
VIA5  ACT10 ACT11 ACT12   (VIA5 直接执行，可持续按住)
```

- 旋钮 A（左上）：逆时针 = 摇杆左 `JOY_LEFT`，顺时针 = 摇杆右 `JOY_RIGHT`。
- 旋钮 B（右上）：逆时针 = 摇杆下 `JOY_DOWN`，顺时针 = 摇杆上 `JOY_UP`。
- 旋钮 C（下方大旋钮）：`ENC_CC` / `ENC_CW`，按下 `ENC_SW`。
- 旋钮 A/B 按下分别是 VIA3/VIA4，读取第一页配置，旋转仍固定为 X/Y。
- 五处均直接执行 VIA，含左上与左下的持续按住。建议 L0 左下 MO(1) 临时普通 L1，松开恢复；普通层 TO(0) 返回。无固定退出，切层不切 USB。
- AG 灯：LED 索引 1/2/4/5/6/7，保持宿主原色；白/灰系 Idle 常亮为宿主亮度的 5%，保留冷暖色调；普通呼吸和任意颜色的 snake 转呼吸均为 0–100%×宿主亮度，跟随宿主速度，0 暂停，不叠加 Idle 降亮。
- 普通 ACT 平时不亮，按住期间点纯白光；两个顶部自定义方键常灭，左下仅跟随环境灯，无新增按压白光。
- 宿主环境灯映射第四排全部四灯（LED 12–15），蓝色类 snake 为 0–10%×宿主亮度呼吸、海绿色类 snake 常亮、其他 snake 为 0–100% 呼吸；不按 App 或精确 RGB 匹配，颜色不替换。速度跟随宿主，0 暂停；显式 breath/solid/shallowBreath 不套这些环境例外。环境提示优先于 ACT10/ACT11/ACT12 按压；海绿/蓝色规则只表示外观，不是独立录音/运行标志。
- 兼容数字 0–6 / 字符串效果、宿主速度、预览和部分字段更新；完整对照与主机测试命令见 `docs/lighting.md`。

## 文件

| 文件 | 说明 |
|---|---|
| `keymap.c` | 四层静态/动态路由、AG/ACT/JOY/ENC、层动作迁移、EEPROM 迁移 |
| `codex_micro_protocol.c/.h` | Codex Channel 2 JSON-RPC（来自上游 v1.3.5，去掉 kb16_config/Channel 3） |
| `poc_mode.c` | 10 秒窗口、BKP 标志、模式复位 |
| `poc_usb.c` | 描述符选择与 raw HID 长度（core patch 的强符号） |
| `poc_via.c` | `via_command_kb()` 路由：VIA 模式放过，Codex 模式吞掉 |
| `poc_power.c/.h` | 共用一小时兜底与 USB 挂起、心跳/重复通信续时；宿主关灯只门控 RGB，不关闭 OLED |
| `oled_icons.c` / `oled_splash.h` | 留边的 Logo、任务状态、居中数字和临时按键页，包含默认层 |
| `oled_input.c` | 解码最终 6KRO/NKRO、消费/系统和鼠标报告，不写屏、不保存输入日志 |

## 默认与升级

- 默认 Codex USB 身份进入 L0，VIA 等待/锁定进入普通 L1；四页可见，普通透明键只继承 L1。
- 原旧 L3→新 L0，旧 L0/L1/L2→新 L1/L2/L3，所有支持的层动作目标一起转换；宏/地址不变。新默认带新标记、已有五处不清空，详见迁移文档。请自行配置返回键。

## 首次上电

`POC_OLED_INPUT` 由当前 keymap 启用，ChibiOS 报告成功入队后只更新 RAM 文本，绘制在 OLED task；其他 keymap 不启用此观察。完整显示语义与测试见 `docs/oled.md`。

固件会在 QMK 用户 EEPROM 里写一个 `KB16` 版本戳；发现戳不匹配（例如刚从别的固件刷过来）就 `eeconfig_init()` 重建动态键位，避免继承旧的 EEPROM 数据。

## 构建

```bash
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```
