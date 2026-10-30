# 开发与构建

## 当前状态

当前源码为 `0.4.8-qmk`（2026-10-30）：VIA 冷启动窗口 **10 秒**，Codex L3 左下角长按 **1 秒**回 L0，其他布局/USB/灯光/休眠语义不变。60 项 ASan/UBSan 离线检查和 QMK 编译通过；本次只构建及发布，**没有刷入新版**。上一版 `0.4.7-qmk` 已刷入并通过版本 RPC，用户随后反馈“目前没问题”；不据此声称新版时长或一小时休眠经过实机测量。

发布包位于 [v0.4.8-qmk](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases/tag/v0.4.8-qmk)，本地另存仓库外 `固件构建/v0.4.8-qmk/`：

| 文件 | 字节 | SHA-256 |
|---|---|---|
| `doio_kb16_rev2_codex_micro_v0.4.8-qmk.bin` | 50,848 | `8deaf0da59ca51b900276e1b3225496559035ce364c5a39e2554ed7efaf7e988` |
| `doio_kb16_rev2_codex_micro_v0.4.8-qmk.hex` | 143,038 | `de78cfecbf3898996cd5108bb84da941ec1a27420313d382edc7a29e143dfa6d` |
| `kb16_codex_via.json` | 974 | `7e062669818127d76bc01ead3cb0bc48f624953f0ede11aae4652a0e7bbe25f6` |

Arm GNU Toolchain 15.3.Rel2、QMK CLI 1.2.0、下述固定 QMK 基线。链接镜像 50,830 字节，四字节对齐 DFU 应用主体 50,832 字节，加 16 字节 DFU suffix 后 BIN 为 50,848；应用向量始于 `0x08002000`，栈指针 `0x20000400`、复位向量 `0x08002239`。已离线核验 DFU suffix/CRC、版本/等待文字、双 Raw HID 描述符和映像地址，未做 Flash 读回。`.data` 1,220、`.bss` 10,348、栈 3,072、剩余堆 5,836 字节，与上一版相同；ELF 无板级 DOIO `render_logo`。

README 的七张 OLED 图从本版测试实际导出的 512 字节帧缓冲生成，包含等待页、1/2/3、GUI+C、Codex Logo 及六任务页；标注为合成预览而非照片。生成命令见根目录 README；测试覆盖 9,999/10,000 ms 窗口、有效 VIA 锁定、999/1,000 ms 长按、边距和时钟回绕。

下方带旧版本号、30 秒或 5 秒时长的构建/刷写日志为历史记录，不代表当前运行参数。

仓库只保留源码和文档，预编译固件（`.bin` / `.hex` / `.uf2`）不入库。

当前唯一实现是 `firmware/keymaps/codex_micro_via_v1/`（QMK + VIA 双模式）：

- `0.3.0-qmk` 只定义四层：Layer 0–2 普通 VIA，Layer 3 固定 Codex。VIA 四页均可编辑保存，但第四页的按键/旋钮不参与运行。
- 上电 10 秒内收到 VIA 命令就锁定 VIA 身份；否则复位进 Codex 身份。
- USB 身份、Raw HID 端口大小、VIA 层数由 `firmware/qmk-core-patches/` 里的 core patch 提供。

`kb16-fw/` 的 Rust/embassy 版本已放弃（实机无法点亮/枚举，且需求已改为 QMK + VIA），保留目录仅作历史参考。

## 目录

```
firmware/keymaps/codex_micro_via_v1/   QMK keymap（唯一实现）
firmware/via/kb16_codex_via.json       VIA 定义
firmware/qmk-core-patches/             需要覆盖到 QMK 树的完整文件
firmware/apply-core-patches.sh         把 patch + keymap 装进 QMK 树
docs/                                  本文档
```

## 构建

### 环境

- QMK commit `f5db962110615fd263c3ebf9f0402e7c2b08fbe4`（`qmk` 1.2.0 验证过）。完整文件 patch 绑定此基线，安装脚本遇到其他 commit 会拒绝覆盖；请使用单独源码树，不要强行切换有改动的 QMK 工作区。
- Arm GNU Toolchain（`arm-none-eabi-gcc`，需要带 newlib `nosys.specs`）
- Python 3（生成/校验文档可选）
- `dfu-util`（刷写）

macOS 上用 Homebrew 的 `arm-none-eabi-gcc` 会因为缺 newlib 的 `nosys.specs` 失败，改用 Arm 官方 toolchain。

### 安装并编译

```bash
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```

生成 `doio_kb16_rev2_codex_micro_via_v1.bin`（当前约 51 KB）。这是本地构建产物，不要提交到仓库；HEX 位于 QMK 的 `.build/`。

### 回归测试

```bash
QMK_HOME=~/qmk_firmware python3 firmware/keymaps/codex_micro_via_v1/tools/run_tests.py
```

需要 Python 3、Clang 和上述 QMK 源码。二十八组灯光、十一组可靠性、十二组 OLED、四组共享休眠、两组启动 hook/模式、存储、VIA 坐标及休眠配置，共 60 项，使用 ASan/UBSan 和真实 QMK keycode 头文件；共享休眠直接编译 `poc_power.c`，USB/按键/迁移/指示器和板级 OLED hook 从生产源码提取。覆盖原有灯光/一小时策略以及首帧无 DOIO、十秒等待/锁定、居中文字和边距，不能代替实机验收；合成画面导出见 `oled.md`。

### 可选校验

```bash
qmk lint -kb doio/kb16/rev2 -km codex_micro_via_v1
```

构建完成后可以检查二进制里两套描述符是否都在：

```bash
python3 - <<'PY'
d = open('doio_kb16_rev2_codex_micro_via_v1.bin', 'rb').read()
print('codex', d.find(bytes.fromhex('3a306083')))
print('codex raw', d.find(bytes.fromhex('0600ff0901a1018506')))
print('via raw', d.find(bytes.fromhex('0660ff0961a1010962')))
PY
```

三个偏移都不应为 `-1`。

## 刷写

```bash
dfu-util -a 2 -d 1EAF:0003 -D doio_kb16_rev2_codex_micro_via_v1.bin
```

进入 Bootloader：拔 USB，按住背面 `RET` 键，再插 USB；`dfu-util -l` 看到 `1EAF:0003` 后松开 `RET`。

如果 `dfu-util` 显示 `UNKNOWN` 或 `-a 2` 返回 `LIBUSB_ERROR_PIPE`，断电重进一次。

### 最新冷启动等待页构建（0.4.7-qmk）

2026-10-30 用户授权刷机，并要求取消 DOIO 页、开机三十秒显示 `VIA WAITTING`。检查发现 QMK `keyboards/doio/kb16/kb16.c` 在用户 renderer 之前强占五秒绘制 DOIO；通过当前 keymap 的 `OLED_LOGO_TIMEOUT=0` 取消该路径，最终 ELF 不再链接 `render_logo`。新增 `poc_mode_via_waiting()` 提供实际等待状态，收到 VIA 命令后显示普通页，无命令三十秒超时后按原逻辑进入 Codex。未改 Bootloader、读保护、四层、灯光/休眠策略或宿主。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_boot_wait_v0_4_7_20261030.bin
SHA-256 70230ce56c341f85d3cf2587cf0d76c12426c6c7fcdf844e74dfadcfccc8affe
```

60 项 ASan/UBSan 离线检查及固定 QMK 基线编译通过。新增实际板级 OLED hook 的首帧/五秒/16 位回绕委托检查、模式等待/锁定/三十秒超时和屏幕文字逐像素/边距检查。链接映像 50,830 字节，对齐下载主体 50,832 字节，带 DFU suffix 的 BIN 为 50,848 字节，同目录有 HEX。`.data` 1,220、`.bss` 10,348、栈 3,072、剩余堆 5,836 字节；原 DOIO 位图/renderer 的无效路径被优化移除。版本、文字、向量表、DFU suffix CRC、双 Raw HID 描述符、源码/构建输入和官方回退 SHA-256 核对通过。

本版已刷入：一次性监测捕获 `1EAF:0003`、唯一 Maple alt 2、USB 路径 `32-1.3.1`，确认 bootloader 名称及 `0x08002000` 后仅下载一次。主体 50,832 字节达到 100% 且 Download done；复位阶段返回 74，未重复下载。随后 Codex `303A:8360` 在新 HID 路径 `DevSrvsID:4295341242` 枚举，sys.version（3.2 ms）和 device.status（3.9 ms）返回 `0.4.7-qmk`，宿主层号 1 对应 QMK L3。日志为 `/tmp/kb16-flash-v0_4_7-20261030.log`。未改 Bootloader/读保护，未做 Flash 读回或注入模拟任务灯；版本确认不代表新页面和一小时策略已完成目视/时序验收。请普通断电重插、不按 RET、不开 VIA，观察是否不再显示 DOIO、三十秒内显示 `VIA WAITTING` 后进入 Codex。若复位沿用 Codex 模式，不据此判定冷启动窗口失效。

### 历史一小时兜底与心跳续时构建（0.4.6-qmk，未刷入）

2026-10-30 用户将规则调整为 VIA 一小时空闲关闭，Codex 跟随下发并有一小时兜底；随后明确纠正心跳和重复报文都要续时。`poc_power.c` 统一控制 OLED/RGB，以输入和最近有效宿主 RPC/VIA 通信中较新的时刻计时。Codex 已接收灯态全部不发光时同步关闭，心跳不能重新点亮，呼吸低点不算关灯；USB 挂起最高优先级。休眠不改灯态或 EEPROM，实际下发与完整边界见 `lighting-compatibility.md`。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_host_power_v0_4_6_20261030.bin
SHA-256 d21f687b46cc63a672c6b8ec2182a574650f7b99bb3bc931282e1e64f4f110f7
```

57 项离线检查及固定 QMK 基线编译通过。链接映像 51,382 字节，对齐下载主体 51,384 字节，带 DFU suffix 的 BIN 为 51,400 字节，同目录有 HEX。`.data` 1,220、`.bss` 10,348、栈 3,072、剩余堆 5,836 字节，相比上版静态 RAM 增 8 字节。十三槽、RGB 原色及微调、USB 身份、四层/30 秒窗口和 OLED 六格布局不变。未刷入、未修改应用或注入模拟灯光；实机仍为 `0.4.2-qmk`，不能把模拟一小时和报文测试当作硬件验收。

### 历史休眠同步与扩展任务灯构建（0.4.5-qmk，未刷入）

2026-10-30 对齐 OLED 与 RGB 空闲时间及输入空闲时钟，默认均不空闲关闭；USB 挂起时二者关闭，恢复时重新点亮未变化的 OLED 帧。任务灯存储/渲染扩为十三槽 id 0–12，映射见 `lighting-compatibility.md`，输入仍为六个原生 AG 和七个 ACT。扩展任务优先于按压/环境，off 释放灯位，RGB 保留原色和已约定的 AG/环境微调。RPC 容量为 2048，并修复 off 遭省略效果的局部更新重新点亮。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_extended_tasks_v0_4_5_20261030.bin
SHA-256 4cb4281f7231084296ade1064f1262553f4be16b390c830e8c5bc6a08b9610bb
```

51 项 ASan/UBSan 离线检查及固定 QMK 基线编译通过。链接映像 50,970 字节，对齐下载主体 50,972 字节，带 DFU suffix 的 BIN 为 50,988 字节，同目录另存 HEX。`.data` 1,220、`.bss` 10,340、栈 3,072、剩余堆 5,844 字节；相比上版 `.bss` 增 1,164，主要用于额外七槽与 RPC 缓冲。OLED 仍为六格，未增加分页。构建产物的向量表、版本、DFU suffix CRC、Codex 身份、双 Raw HID 描述符及源码/构建输入一致性均通过检查；VIA 身份为运行时生成，另核对生成源码，不以二进制连续字节搜索替代校验。未刷机、未注入灯光、未修改应用；已安装 Cindy 仍只下发六路，扩展槽待宿主配套及实机验收。

### 历史 AG snake 呼吸构建（0.4.4-qmk，未刷入）

2026-10-30 用户将 AG 任意颜色的 snake 从固定快闪改为呼吸。复用现有原生 breath 的 0–100%×B 波形和宿主速度，0 暂停当前相位；不改 RGB、不回写原始效果和亮度，不对 AG 白灰 snake 叠加 Idle 常亮 5%。删除固定 5 Hz 分支与周期常量。第四排蓝色 snake 0–10%×B 呼吸、海绿 snake 常亮及其他环境效果、原生效果、OLED、四层和 30 秒 VIA 窗口均不变。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_ag_snake_breath_v0_4_4_20261030.bin
SHA-256 b98e873c83fe57f9bfd5008d7395d2faf8667b040931bc5b284c6980e0d731be
```

42 项 ASan/UBSan 离线检查和固定 QMK 基线编译通过。AG snake 检查数字/字符串、六槽、完整周期、三档亮度、速度变更/暂停/恢复、缺省速度、off 及 4096 组 RGB 透传，现有环境、按键、OLED 和 VIA 回归保留。链接映像 50,978 字节，对齐下载主体 50,980 字节，带 DFU suffix 的 BIN 为 50,996 字节，同目录有 HEX。`.data` 1,220、`.bss` 9,176、栈 3,072、剩余堆 7,008 字节不变。向量表、版本、DFU suffix CRC、二进制中的 Codex 身份/双 Raw HID 描述符和源码/构建输入一致性通过检查；VIA 身份在运行时生成，另核对其生成源码。未刷机、未注入模拟灯光、未修改宿主；实机仍为 `0.4.2-qmk`，待用户目视验收。

### 历史通用颜色识别构建（0.4.3-qmk，未刷入）

用户明确不应继续维护 GPT/Cindy 的精确 RGB 白名单，宿主的颜色、亮度、效果、速度才是原始依据；本轮确认保留 AG Idle 5% 与跑马转快闪，环境本地调整保留。生产代码删除所有 App 色值常量，通用整数 HSV 分类只返回色类；`apply_light_adjustments()` 仅修改本次输出副本，不回写宿主 c/b/e/s。暖白/冷白/灰保留各通道比例，蓝色类环境 snake 为 0–10%×B 呼吸，海绿色类 snake 保留常亮，显式 breath/solid/shallowBreath 不套这些环境例外。OLED 标签是灯色推断，不是独立业务状态；主题/品牌预览/录音可能误判，详见 `lighting.md`。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_generic_color_v0_4_3_20261030.bin
SHA-256 e21dd8d19c56de303839df16d4e79ff55de6df7aabb8c25048e9fd8ba6e12a83
```

40 项 ASan/UBSan 离线检查及固定 QMK 基线目标编译通过。新增通用颜色、深浅色、冷暖白/灰、海绿色变体、黑色基色有效动画、宿主参数隔离和 OLED 推断像素检查；4096 组 RGB 在 AG/环境原生 breath 下验证 63% 宿主亮度及暂停相位不被分类改写。链接映像 50,994 字节，对齐下载主体 50,996 字节，带 DFU suffix 的 BIN 为 51,012 字节，同目录有 HEX。`.data` 1,220、`.bss` 9,176、栈 3,072、剩余堆 7,008 字节，与已刷版相同；没有动态分配。版本、向量表、DFU suffix CRC、双描述符、源码/构建输入一致性通过检查，另核对生产源文件不再含旧 App 色值常量。未刷机、未向实机下发模拟灯光、未修改宿主 App；实机最后确认仍为 `0.4.2-qmk`，不能把本版离线结果当成实机验收。

### 历史 Cindy 颜色兼容构建（0.4.2-qmk，已刷入）

修复用户反馈的第四排运行蓝光未明显降亮、OLED 状态一直 OTHER。本机已安装 Cindy 构建（sourceCommit `b3dd8436228135aa8be9827d204e4e5eaf9316b4`）使用运行 `#4C6FFF`、等待 `#FFA000`、完成 `#35C759`、错误 `#FF453A`，不是旧代码只识别的 GPT 色。新增精确状态色别名，保留所有宿主原色；两套运行蓝色 + 环境 snake 均为 0–10%×宿主亮度呼吸。Cindy 默认环境 B=0.7 时峰值 7%，AG 原幅度不变；录音、白色处理、其他效果、品牌预览/未知色均不扩展降亮或状态猜测。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_cindy_palette_v0_4_2_20261030.bin
SHA-256 122a1c9347556ce819b19a56253afd4d4475c76922129d1d63d62f70449e7ad0
```

35 项离线检查及固定 QMK 基线目标编译通过，新增回放在修复前可复现运行蓝光未被降亮，修复后通过；另覆盖 Cindy 状态语义与实际 OLED 标签像素。链接映像 50,910 字节，下载主体对齐为 50,912 字节，带 DFU suffix 的 BIN 为 50,928 字节；同目录有 HEX。`.data` 1,220、`.bss` 9,176、栈 3,072、剩余堆 7,008 字节，不增加静态 RAM。向量表、DFU suffix CRC、版本、双描述符和源码/构建输入一致性检查通过。

2026-10-30 16:40 经已启动的 RET 监测捕获 `1EAF:0003`，确认 STM32duino alt 2、应用地址 `0x08002000` 和 USB 路径 `32-1.3.1` 后只下载一次，主体 50,912 字节达到 100%。复位返回 `74` 后不重复刷写；随后 Codex `303A:8360` 在新 HID 路径 `4295331337` 枚举，Cindy 灯光宿主也记录重新连接。16:41 的 `sys.version`（4.8 ms）和 `device.status`（3.9 ms）均返回 `0.4.2-qmk`，首次核验宿主层号 1 对应 QMK L3。目标和官方回退镜像哈希已核对，未改 Bootloader/读保护，未做 Flash 读回。刷后再跑 35 项离线检查全部通过；未修改/重启 App，也未发送模拟任务/灯光，保留真实宿主状态。第四排蓝光幅度、真实任务标签和录音等视觉表现仍待用户确认，不能把 RPC 成功视为全部实机验收。

### 历史运行环境降亮构建（0.4.1-qmk，已刷入）

仅把第四排当前任务运行提示（ambient/underglow 蓝 `#304FFE` + snake）改为 0–10%×宿主亮度同步呼吸；宿主亮度 50% 时峰值为 5%。录音海绿仍按宿主亮度常亮，白色处理和其他颜色的 snake 仍为 0–100% 呼吸，蓝色的显式 solid/breath/shallowBreath 不变。保留 `0.4.0-qmk` 的 OLED、四层、EEPROM、切层白光/5 秒退出和 30 秒启动逻辑。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_running_ambient10_v0_4_1_20261030.bin
SHA-256 a99a05243b5b4fe830c93543d64995808b8ee3a162c9692a6dac5346b206d720
```

32 项离线检查及固定 QMK 基线目标编译通过；新增数值/字符串效果、0/50/100% 宿主亮度和范围隔离检查，确认录音、处理、其他颜色/效果、AG 及局部更新不受运行降亮影响。链接映像 50,858 字节，下载主体对齐为 50,860 字节，带 DFU suffix 的 BIN 为 50,876 字节；同目录有 HEX。`.data` 1,220、`.bss` 9,176、栈 3,072、剩余堆 7,008 字节，与上一版相同。向量表、DFU suffix CRC、版本、双描述符及源码/构建输入一致性检查通过。

2026-10-30 16:27–16:28 经 RET 捕获 `1EAF:0003`，确认 STM32duino alt 2 为应用地址 `0x08002000`、USB 路径 `32-1.3.1` 后仅下载一次，主体 50,860 字节达到 100%。下载完成后复位返回 `74`，未重复写入；随后 Codex `303A:8360` 在新 HID 路径 `4295301253` 枚举。`sys.version`（2.2 ms）和 `device.status`（3.9 ms）均返回 `0.4.1-qmk`，宿主层号 1 对应 QMK L3。未修改 Bootloader/读保护；刷前官方回退镜像 SHA-256 `0d1f21580b81d88f0e9e4a1bb87ae5a7a96f72677d12864651b6a0624cccac01` 已核对。尚未做 Flash 读回或 VIA 配置逐项检查；本轮未发送模拟灯光报文，新第四排呼吸幅度及 OLED 画面仍待用户目视验收，不能仅凭版本应答判定这些功能已通过。

随后只读复查仍返回 `0.4.1-qmk`，状态层号变为 2（QMK L0，普通第 1 层）。这是运行中的层状态变化，不是刷写失败；初次版本核验时的 L3 不是持续锁定。观察 Codex 灯效前应先用 `TO(3)` 返回 L3，不能把普通层 RGB 当作任务环境灯。

用户随后确认第四排已呼吸、AG 与 OLED 基本显示正常，但感觉蓝光并非低亮度、任务状态一直 OTHER；这不是运行降亮或标签语义已通过。只读复查版本仍为 `0.4.1-qmk`、L3；定位到已安装 Cindy 的另一套 RGB，修复见上方 `0.4.2-qmk`。

### 历史 OLED/实时键值构建（0.4.0-qmk，未刷入）

按最新用户确认实现：左右至少 6 px、上下至少 4 px 留边；切入 L3 显示旧终端 Logo+CODEX 约 1 秒，再显示已有六个 AG 的状态。普通 L0–L2 居中显示 1/2/3；成功入队的实际 USB 输出临时替换数字，按住保留，全部松开或瞬时动作后约 900 ms 恢复数字。长提示最多两行；使用 6KRO/NKRO、媒体/系统和鼠标报告，跟随 VIA 改键、宏和双功能键，不显示输入法/应用最终文字，不保存输入历史。包含第四排环境呼吸及录音海绿常亮，四层、EEPROM、切层和 30 秒启动规则不变。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_oled_live_keys_v0_4_0_20261030.bin
SHA-256 cdd03632e0a08ce5befffaa02f01b058a3994c0d55aeb7227249f7fa19db02bb
```

30 项离线检查和当前 QMK 目标编译通过；未启用 `POC_OLED_INPUT` 的上游 `codex_micro_upstream` 也重新编译通过（49,272 字节），新增观察默认不影响其他 keymap。当前链接映像 50,826 字节，下载主体对齐为 50,828 字节，带 DFU suffix 的 BIN 为 50,844 字节；同目录有 HEX。`.data` 1,220、`.bss` 9,176、栈 3,072、剩余堆 7,008 字节；keymaps 160、encoder_map 48 字节。向量表、suffix CRC、双描述符、版本和源码/构建树一致性通过检查；六张合成画面已查看，像素边距通过，不代表物理屏幕遮边已实测。当前未刷机，实机仍为 `0.3.0-qmk`。

### 历史环境呼吸构建（0.3.2-qmk，未刷入）

第四排四灯的环境 snake 提示由快闪改为 0–100%×宿主亮度同步呼吸，速度跟随宿主（0 暂停）。蓝色运行、白色处理提示呼吸；录音海绿 snake 继续常亮、完成提示常亮，六个 AG 的 snake→5 Hz 快闪不变。原色、切层按住白光/5 秒退出、四层和 30 秒启动规则不变。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_ambient_row4_breath_20261030.bin
SHA-256 a82bcef29f936b876729e0bf7271faf3a6046431cd16090371f25bbec7ec58fb
```

25 项离线检查和固定 QMK 基线编译通过；覆盖四灯同步呼吸的低点/半亮/峰值/回落、全局亮度、数字/字符串效果、preview、速度与暂停、录音常亮及 AG 隔离。链接映像 47,234 字节，下载主体对齐为 47,236 字节，带 DFU suffix 的 BIN 为 47,252 字节；同目录有 HEX。`.data` 1,216、`.bss` 7,952、栈 3,072、剩余堆 8,240 字节，keymaps 160、encoder_map 48 字节。向量表、DFU suffix CRC、版本、双描述符和源码/构建树一致性通过检查。尚未刷入，实机仍为 `0.3.0-qmk`，不能将离线结果当作新灯效目视验收。

### 历史环境四灯构建（0.3.1-qmk，未刷入）

只把 ambient/underglow 从 MIC 两灯扩为第四排全部 LED 12–15（切层键、ACT10/ACT11/ACT12）；保留原色、亮度、录音海绿常亮和一般 snake→5 Hz 同步快闪。环境提示优先于该排 ACT 按压白光，切层键按住白光仍优先，5 秒退出保持。未改 USB、四层、EEPROM 或 30 秒启动逻辑，未实现此前讨论的直接 Codex/双接口方案。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_ambient_row4_20261030.bin
SHA-256 345b0fc0d2bd1db9002555ea13195ac9437307be823aeac6ad0e9a185bfed8de
```

24 项离线检查及固定 QMK 基线编译通过。链接映像 47,246 字节，下载主体对齐为 47,248 字节，带 DFU suffix 的 BIN 为 47,264 字节；同目录有 HEX。`.data` 1,216、`.bss` 7,952 字节，栈 3,072、剩余堆 8,240 字节，与上一版一致；keymaps 160、encoder_map 48 字节。向量表、DFU suffix CRC、版本、双描述符和源码/构建树一致性通过检查。实机仍为下面的 `0.3.0-qmk`，四灯效果尚未刷写/目视验收。

### 已刷四层构建（0.3.0-qmk）

L0–L2 执行普通 VIA 改键，L3 固定 Codex；VIA 显示四页且均可保存读回，但第四页不改变实际按键/编码器。默认 Codex L3，`TO(3)` 返回，L3 左下角长按 5 秒退 L0，OLED 只显示实际 0–3 层号。包含录音海绿常亮及此前协议/灯光修复，30 秒身份选择逻辑不变。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_four_layers_codex_l3_20261030.bin
SHA-256 c195f101db1aee166cebc70fda3623e56adde72cea078773910210446d236e5c
```

2026-10-30 已通过 22 项离线检查和固定 QMK 基线编译。链接映像 47,230 字节，对齐后下载主体 47,232 字节，带 DFU suffix 的 BIN 为 47,248 字节；同目录有 HEX。`.data` 1,216、`.bss` 7,952 字节，栈预留 3,072 字节，剩余堆 8,240 字节。ELF 中 keymaps 为 160 字节、encoder_map 为 48 字节，均为四层。向量表、DFU suffix CRC、版本、双 Raw HID 描述符及源码/构建树一致性通过检查。

旧键位/旋钮/宏的 EEPROM 起址保持，未使用空间只作地址兼容，不是第五层。一次迁移普通 L0–L2 中目标旧 L4 的 TO/MO/DF/PDF/TG/OSL/TT/LT/LM 动作到 L3，第四页和其他键值不重置。测试覆盖占位页保存但不执行、后续编辑不重复覆盖、默认层/RGB/OLED 与 L3 长按退出。刷前仍须导出 VIA 配置；QMK/VIA 自身配置有效性检查在其他构建/导入场景下仍可能重置配置。

2026-10-30 已经 RET/Maple DFU 刷入：`1EAF:0003`、alt 2、路径 `32-1.3.1`，下载主体 47,232 字节达到 100%，未改 bootloader/读保护。复位步骤返回 `74`，未重复刷写；随后观察到 VIA `D010:1601`（新路径 `4295290744`）及自动进入 Codex `303A:8360`（新路径 `4295290822`）。`sys.version` / `device.status` 独立请求分别于 2.3 / 3.0 ms 返回 `0.3.0-qmk`，默认宿主层号 1 即 QMK L3。未做 flash 读回。

按长按退出、旋钮返回的测试指令后，只读 `device.status` 连续监测捕获实际 L3→L0→L1→L2→L3，变化约在监测启动后 22.211 / 24.914 / 25.395 / 25.873 秒。用户随后确认长按 5 秒回 L0、旋钮 B 三次按压依次进入 L1/L2/L3，OLED 编号均正确，与协议状态一致；不是独立精测按键按住时间。临时检测已结束，不修改键位、层或灯光，不留常驻程序。VIA 第四页读写、旧配置全量保留和新录音常亮仍待各自验收。下面旧五层记录不能替代新版其他项目验收。

### 历史：已刷修复构建（0.2.0-qmk，五层）

`0.2.0-qmk`：AG 不依赖灯光分配、编码器不重复、VIA 报告边界/写入保护、控制 SET_REPORT、OLED 六槽状态及切层返回修正；Idle 常亮 5%，普通常亮 100%，呼吸 0–100%，均尊重宿主全局亮度。新默认 L3 返回 L4，不覆盖已保存的 VIA 配置。

产物在仓库外，2026-10-30 已通过 RET/DFU 刷入：

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_fixed_idle5_breath100_20261030.bin
SHA-256 78a520ff5900b1c86b4ec56b7c8c5c431eca0702755ef160644807655b668948
```

主体 49,844 字节，带 DFU suffix 的 BIN 为 49,860 字节；同目录有 HEX。ELF 的 `.data` 为 1,220、`.bss` 为 8,072 字节，栈预留 3,072 字节，剩余堆 8,112 字节。向量表、两套 USB 描述符、源码与 QMK 构建树一致性均通过检查。首刷 `15472a93…` 是旧的 10% 灯光对照版，不是这个修复构建。

实机记录：捕获 `1EAF:0003` 的 STM32duino alt 2，下载主体 49,844 字节达到 100%；`dfu-util` 完成下载后复位返回 `74`，随后 Codex Raw HID 以新设备路径重新枚举。`sys.version` 和 `device.status` 均返回 `0.2.0-qmk`，并非只凭本地构建判断已更新。四项 RPC 应答约 2.1/3.0/3.0/6.9 ms；用户已分别目视确认八组灯光，包括 AG 基色/Idle/呼吸、半亮/全灭、本地按住反馈及 MIC 三色快闪/白色常亮/提示清除。完整记录见 `lighting.md`。未做 flash 读回或光度测量，其余物理交互仍需逐项验收。本次 RET 已成功进入 Maple DFU，不需要为此次刷写改试 BOOT。

### 历史：录音常亮构建（0.2.1-qmk，五层）

仅将 ACT10/ACT11 的海绿录音 `snake` 提示改为常亮，颜色、亮度、关闭/清除和 AG 灯效保持原规则。19 项离线检查和 QMK 编译通过，尚未刷入；用户正在录音时不自动重启设备。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_recording_solid_20261030.bin
SHA-256 c1f8123cb6c5b2d16080152bec4f0a73175fc752cfeac02842b3d6a13e8a2690
```

主体 49,860 字节、BIN 49,876 字节；同目录有 HEX。`.data` 1,220、`.bss` 8,072 字节，栈/堆预算不变。实际生效需另行 RET/DFU 刷入；此前 `0.2.0-qmk` 的海绿快闪目视结果不代表这个常亮变更已实测。

### 历史：层号/返回修复版（0.2.2-qmk，五层，未刷）

包含 `0.2.1-qmk` 的录音常亮；首次以 Codex 身份启动时对旧默认旋钮 B 的四层循环进行一次性检查，仅将 L3 的 `TO(0)` 改为 `TO(4)`，保留其他键位、旋转配置和宏。自定义循环不自动修改，之后 VIA 再改的内容不反复覆盖。L4 长按 5 秒退回 L0 在两种 USB 身份均可使用，但不改变身份。

OLED 仅显示 `LAYER 0`–`LAYER 4`，不再显示图标、任务标签、功能名称或弹窗。20 项离线检查与 QMK 编译通过，尚未刷入。

```text
/Users/chenzhuyu/Downloads/海康威视/固件构建/doio_kb16_rev2_codex_micro_via_layer_return_oled_20261030.bin
SHA-256 2246f1827862fa0793e39f04277de535a3ae4e372e8cac2b0e52f9aa94b7a5e5
```

链接映像 46,894 字节，加入 2 字节对齐后烧写数据为 46,896 字节，带 DFU suffix 的 BIN 为 46,912 字节；同目录有 HEX。`.data` 1,216、`.bss` 7,960 字节，栈预留 3,072 字节，剩余堆 8,232 字节。

迁移测试覆盖旧默认循环只改一个键、布局标志和其他键不变、自定义循环不改、已经含 `TO(4)` 不改、迁移后用户再改不会被覆盖；OLED 测试覆盖 0–4 层即时刷新、任务更新和弹窗不再遮挡层号。实机需检查 `sys.version` 为 `0.2.2-qmk`，再长按左下角进入 L0、按旋钮 B 经 L1/L2/L3 返回 L4，并核对 OLED 实际编号。

## 硬件验收

刷入后按顺序验证；先区分下载、重新枚举和具体功能的结果，不能因复位返回非零就重复盲刷。无法枚举时使用 RET 回退官方应用固件，绝不擦 Bootloader：

1. 冷启动 30 秒内打开 VIA，能识别 `D010:1601`；改一个 Layer 0 键位并测试。
2. 拔插后不打开 VIA，30 秒后设备以 `303A:8360` 重新枚举，ChatGPT 能识别为 Codex Micro。
3. 6 个 AG 键事件正常，6 个任务灯跟随 AG 位置。
4. ACT06-09/12、ACT10/ACT11 正常。
5. 旋钮 C 的 `ENC_CC/CW/SW` 正常，每格仅一个旋转事件；旋钮 A/B 触发 X/Y 四方向摇杆。
6. L3 左下角长按 5 秒进入 L0，普通层通过 VIA 配置的 `TO(3)` 返回 L3；没有可进入的 L4。核对旧 L4 目标迁移且其他键位/宏未重置。
7. VIA 显示四页，第四页按键/旋钮可写入读回，但运行仍为固定 Codex；前三页改键实际生效。
8. OLED 留边，Codex 切入 Logo+CODEX 后进入六任务页，普通层为居中 1/2/3，实际输出键临时替换、松开约 900 ms 恢复；核对默认层、VIA 改键、宏/双功能/媒体/鼠标提示不失真。按 `lighting.md` 核对第四排环境呼吸、录音海绿常亮及 AG；未实测不得写成通过。
9. 能进入 Bootloader 并刷回官方 VIA 固件。

## 提交

- commit：`type(scope): 简述`，中文，≤50 字符，结尾不加句号。
- 一个 commit 只做一件事。
- 同步更新 `CHANGELOG.md` 和相关文档。
- 需求/决策/踩坑回写到 `AGENTS.md` 或 `docs/`。
