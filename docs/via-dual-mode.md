# QMK + VIA 双模式

当前源码 `0.6.0-qmk`：Codex 调整到第一页 L0，L1–L3 普通 VIA；取消固定长按退出，切层完全由用户配置。流水线版已刷入，版本应答为 0.6.0-qmk，当前 Codex L0；用户随后整体反馈正常，未逐项精测。

## 四层模型

| VIA 编辑页 | 固件层 | 实际运行 |
|---|---|---|
| 第一页 | L0 | 固定 AG/ACT/JOY/ENC + 五处 VIA 自定义 |
| 第二页 | L1 | 普通动态键位/encoder map，普通透明继承基底 |
| 第三页 | L2 | 普通动态键位/encoder map |
| 第四页 | L3 | 普通动态键位/encoder map |

`DYNAMIC_KEYMAP_LAYER_COUNT=4`、`VIA_VISIBLE_LAYER_COUNT=4`、`POC_CODEX_LAYER=0`，无第五层。L0 五处为 R1C1、R1C4、A/B 按下、R4C1；全部直通 QMK，无长按定时、短按重放或退出白光。其余原生映射固定，见 [键位图](keymap.md)。

推荐 Codex 的左下角配 `MO(1)`：按住普通 L1、松开 Codex；普通层配 `TO(0)` 返回。可用 `TO(1/2/3)` 常驻普通层。保留 QMK 原生高层优先规则，不引入低层强制覆盖引擎。普通透明键继承普通 L1 而不是原生 L0，源层缓存负责跨层松开。

## 启动与 USB

```text
真正上电 → 普通 L1 / VIA WAITTING
 ├─ 10 秒内有效 VIA 命令 → VIA 身份锁定直到断电
 └─ 没有 VIA 命令 → soft reset → Codex USB 身份，默认 L0
```

| | VIA 身份 | Codex 身份 |
|---|---|---|
| VID:PID | `D010:1601` | `303A:8360` |
| 厂商 / 产品 | DOIO / KB16-01 | Work Louder / Codex Micro |
| Raw HID | `FF60:0061`，32 字节，无 ID | `FF00:0001`，Report ID 6，64 字节 |
| 通信 | VIA 四页读写 | Codex Channel 2 JSON-RPC |

- 有效协议查询即可锁定，错误长度、未知 ID、越界层不得锁定；只打开页面或串口不算。
- 切层不改变 USB 身份。VIA 锁定时在 L0 也不能使用 Codex 接口；须断电，让窗口超时。
- 模式标记在 `BKP->DR9`，保留 bootloader 的 `DR10`；超时软复位，不停留 DFU。
- Codex 接收含 ID 的 64 字节及去 ID 的 63 字节；控制 SET_REPORT 入队后主循环解析，不在中断写 EEPROM/解析 JSON。无 Mapper/Channel 3 或 Input `fs.*`。
- RPC `device.status.layer_index` 为实际 QMK 层 + 1，1=Codex、2/3/4=普通层。

## 配置与显示

十秒内用 VIA 读取后可一直编辑；第一页编辑指定五处，第二至第四页编辑普通整层。改完断电重插、不连 VIA，十秒后进入 Codex，继续使用保存的配置。[完整方法及迁移](codex-via-keys.md)。

普通层 OLED 为 1/2/3，对应 L1/L2/L3（VIA 第 2/3/4 页），与 RPC/VIA 页号分开编号；Codex 为 Logo/任务页。普通 HID 输出临时显示键名，松开约 900 ms 恢复；安全边距、六格状态、任务全灭不关屏、一小时空闲/心跳续时不变。RGB 仍遵循宿主原色及既定微调，第四排环境 LED12–15 不变；不再有专用退出白光。

## 旧配置与构建

旧 L3→新 L0，旧 L0/L1/L2→新 L1/L2/L3，包括键位、encoder map 和层动作目标；已有五处不再清空。宏、EEPROM 起址及容量不变，新默认不旋转。刷前备份，导入旧顺序布局前自己转换；不保证断电中断迁移或任意降级升级安全。

```bash
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```

固定 QMK `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`。core patch 包括 USB 双描述符、报告边界/控制队列、VIA 可见层数、WS2812 PWM DMA，以及仅当前双模式启用的普通层透明继承隔离；未改 QMK layer_on/off/invert/xor 的标准语义。

本地构建只作开发验证，正式镜像由 GitHub Actions 生成；本次验证后提交推送，随后按用户另行要求刷入对应流水线 Artifact 并核验版本；用户确认正常后另行授权以 v0.6.0-qmk 发布。历史 CI/实机记录、官方回退镜像和哈希见 [开发](development.md)，拆机使用内部 RESET 的方法见 [使用与刷写](usage.md)。
