# codex_micro_via_v1

QMK + VIA 双模式 keymap，目标硬件 DOIO KB16-01 Rev2（STM32F103xB，128 KB Flash）。

当前源码 `0.4.8-qmk`：VIA 冷启动窗口 10 秒、L3 左下角长按 1 秒回 L0，等待显示 `VIA WAITTING`，无 DOIO 页。60 项离线检查及编译通过，本版未刷入；上一版 `0.4.7-qmk` 已刷并获用户正常反馈。休眠、灯态、六格 OLED 和四层不变；完整双语指南及展示见仓库根 README。

## 模式

- 上电 10 秒内收到 VIA raw HID 命令 → 保持 VIA 身份 `D010:1601`，Layer 0-3 可改，直到断电。
- 10 秒内没有 VIA 命令 → `soft_reset_keyboard()` → Codex 身份 `303A:8360`。
- 模式标志存在 `BKP->DR9`（`BKP->DR10` 属于 STM32duino bootloader）。

## 层

| 层 | 来源 |
|---|---|
| 0-2 | QMK 动态键位（VIA 读写并执行） |
| 3 | 静态 Codex 层；VIA 第四页仍可保存读回，但运行时不使用所存键位/旋钮 |

`DYNAMIC_KEYMAP_LAYER_COUNT=4`、`VIA_VISIBLE_LAYER_COUNT=4`、`POC_CODEX_LAYER=3`，没有可进入的第五层；旧编码器/宏存储地址保留，不是第五个运行层。

## Layer 3

```text
——    AG00  AG01  ——      (左上/右上键位空出)
AG02  AG03  AG04  AG05
ACT06 ACT07 ACT08 ACT09
切层  ACT10 ACT11 ACT12   (第 4 行右移一格；短按不动作，长按 1 秒切层)
```

- 旋钮 A（左上）：逆时针 = 摇杆左 `JOY_LEFT`，顺时针 = 摇杆右 `JOY_RIGHT`。
- 旋钮 B（右上）：逆时针 = 摇杆下 `JOY_DOWN`，顺时针 = 摇杆上 `JOY_UP`。
- 旋钮 C（下方大旋钮）：`ENC_CC` / `ENC_CW`，按下 `ENC_SW`。
- 旋钮 A/B 按下无动作。
- 切层键 `R4C1`：L3 长按 1 秒进入 L0；普通层用 VIA 配置的 `TO(3)` 返回，不强占普通层同一按键。切层不切 USB 身份。
- AG 灯：LED 索引 1/2/4/5/6/7，保持宿主原色；白/灰系 Idle 常亮为宿主亮度的 5%，保留冷暖色调；普通呼吸和任意颜色的 snake 转呼吸均为 0–100%×宿主亮度，跟随宿主速度，0 暂停，不叠加 Idle 降亮。
- 普通 ACT 平时不亮，按住期间点纯白光；左下角切层键按住时也点纯白光（LED 12）。空位保持熄灭。
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
| `poc_power.c/.h` | 共用一小时兜底、心跳/重复通信续时、宿主关灯和 USB 挂起优先级 |
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
