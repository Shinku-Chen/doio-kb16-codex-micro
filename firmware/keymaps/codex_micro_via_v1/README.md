# codex_micro_via_v1

QMK + VIA 双模式 keymap，目标硬件 DOIO KB16-01 Rev2（STM32F103xB，128 KB Flash）。

开发源码 `0.5.0-qmk`：L3 新开放两个顶部方键、A/B 按下及左下角，共五处 VIA 自定义，OLED 显示实际 HID 输出、松开约 900 ms 恢复。首次升级仅清空这五处，复用原迁移标记而不移动 EEPROM。左上方键短按执行 VIA1、长按一秒回 L0；左下角 VIA5 可持续按住，不再强制切层。切层白光随按钮移到 LED 0，第四排环境仍为 LED 12–15；原生键、旋转、C 按下、USB/灯色灯效/休眠不变。新版未刷入，设备仍为下述 0.4.9；详见 `docs/codex-via-keys.md`。

已发布/实机基线 `0.4.9-qmk`：OLED 不再跟随 Codex RGB 全灭关屏，无任务灯分配时保留 Logo，入层从首个可见帧展示完整约 1 秒。保留一小时静默/USB 挂起息屏、有效心跳及重复报文续时；RGB、四层、EEPROM、10 秒启动和当时左下角一秒退出不变。63 项固件离线检查通过，流水线版本已刷入，版本/切层及用户入层 Logo 目视确认通过；一小时息屏/按键唤醒按用户选择未实测。完整双语指南及展示见仓库根 README。

## 模式

- 上电 10 秒内收到 VIA raw HID 命令 → 保持 VIA 身份 `D010:1601`，Layer 0-3 可改，直到断电。
- 10 秒内没有 VIA 命令 → `soft_reset_keyboard()` → Codex 身份 `303A:8360`。
- 模式标志存在 `BKP->DR9`（`BKP->DR10` 属于 STM32duino bootloader）。

## 层

| 层 | 来源 |
|---|---|
| 0-2 | QMK 动态键位（VIA 读写并执行） |
| 3 | 原生 Codex 固定；第四页仅五处自定义读取 VIA，左上保留长按退出 |

`DYNAMIC_KEYMAP_LAYER_COUNT=4`、`VIA_VISIBLE_LAYER_COUNT=4`、`POC_CODEX_LAYER=3`，没有可进入的第五层；旧编码器/宏存储地址保留，不是第五个运行层。

## Layer 3

```text
VIA1  AG00  AG01  VIA2    (VIA1 短按执行，长按 1 秒回 L0)
AG02  AG03  AG04  AG05
ACT06 ACT07 ACT08 ACT09
VIA5  ACT10 ACT11 ACT12   (VIA5 直接执行，可持续按住)
```

- 旋钮 A（左上）：逆时针 = 摇杆左 `JOY_LEFT`，顺时针 = 摇杆右 `JOY_RIGHT`。
- 旋钮 B（右上）：逆时针 = 摇杆下 `JOY_DOWN`，顺时针 = 摇杆上 `JOY_UP`。
- 旋钮 C（下方大旋钮）：`ENC_CC` / `ENC_CW`，按下 `ENC_SW`。
- 旋钮 A/B 按下分别是 VIA3/VIA4，读取第四页配置，旋转仍固定为 X/Y。
- 切层键 `R1C1`：L3 短按松开执行 VIA1，长按 1 秒进入 L0 不执行短按；`R4C1` 直接使用 VIA5，无固定长按。普通层用 VIA 配置的 `TO(3)` 返回，不强占普通层同一按键。切层不切 USB 身份。
- AG 灯：LED 索引 1/2/4/5/6/7，保持宿主原色；白/灰系 Idle 常亮为宿主亮度的 5%，保留冷暖色调；普通呼吸和任意颜色的 snake 转呼吸均为 0–100%×宿主亮度，跟随宿主速度，0 暂停，不叠加 Idle 降亮。
- 普通 ACT 平时不亮，按住期间点纯白光；左上角切层键按住时也点纯白光（LED 0），受原 RGB 电源策略约束。右上角灭，左下角仅跟随环境灯，没有新增按压白光。
- 宿主环境灯映射第四排全部四灯（LED 12–15），蓝色类 snake 为 0–10%×宿主亮度呼吸、海绿色类 snake 常亮、其他 snake 为 0–100% 呼吸；不按 App 或精确 RGB 匹配，颜色不替换。速度跟随宿主，0 暂停；显式 breath/solid/shallowBreath 不套这些环境例外。环境提示优先于 ACT10/ACT11/ACT12 按压，切层键按住白光仍优先；海绿/蓝色规则只表示外观，不是独立录音/运行标志。
- 兼容数字 0–6 / 字符串效果、宿主速度、预览和部分字段更新；完整对照与主机测试命令见 `docs/lighting.md`。

## 文件

| 文件 | 说明 |
|---|---|
| `keymap.c` | 四层静态/动态路由、AG/ACT/JOY/ENC、1 秒退出、EEPROM 迁移 |
| `codex_micro_protocol.c/.h` | Codex Channel 2 JSON-RPC（来自上游 v1.3.5，去掉 kb16_config/Channel 3） |
| `poc_mode.c` | 10 秒窗口、BKP 标志、模式复位 |
| `poc_usb.c` | 描述符选择与 raw HID 长度（core patch 的强符号） |
| `poc_via.c` | `via_command_kb()` 路由：VIA 模式放过，Codex 模式吞掉 |
| `poc_power.c/.h` | 共用一小时兜底与 USB 挂起、心跳/重复通信续时；宿主关灯只门控 RGB，不关闭 OLED |
| `oled_icons.c` / `oled_splash.h` | 留边的 Logo、任务状态、居中数字和临时按键页，包含默认层 |
| `oled_input.c` | 解码最终 6KRO/NKRO、消费/系统和鼠标报告，不写屏、不保存输入日志 |

## 首次上电

`POC_OLED_INPUT` 由当前 keymap 启用，ChibiOS 报告成功入队后只更新 RAM 文本，绘制在 OLED task；其他 keymap 不启用此观察。完整显示语义与测试见 `docs/oled.md`。

固件会在 QMK 用户 EEPROM 里写一个 `KB16` 版本戳；发现戳不匹配（例如刚从别的固件刷过来）就 `eeconfig_init()` 重建动态键位，避免继承旧的 EEPROM 数据。

## 构建

```bash
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```
