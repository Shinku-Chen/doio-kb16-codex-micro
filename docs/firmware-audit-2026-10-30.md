# 固件对照检查：2026-10-30

检查对象为当前工作区的 `firmware/keymaps/codex_micro_via_v1/`、core patches 和 VIA 定义，包含尚未提交的改动。本次只检查、编译和离线复现；没有修改固件行为，也没有刷机。

> 本报告原始发现保留作历史记录，下文旧位置可能随修复移动。2026-10-30 后续修复见下表：灯光对照版之后，`0.2.0-qmk` 修复版 `78a520ff…` 也已刷入并由实机返回版本确认。第一组位置、状态色、Idle 5% 常亮和 0–100% 呼吸已目视通过；表中离线结果仍不能替代其他输入、VIA/OLED 等实机验收。完整规则见 `lighting.md`。

## 后续修复状态

用户最终改为四层：`0.3.0-qmk` 的 L0–L2 普通 VIA、L3 固定 Codex，VIA 第四页可编辑保存但不执行，OLED 只显示 0–3。旧 L4 目标会一次迁移到 L3，旧编码器/宏起址保持。22 项检查与编译通过，已刷入并收到版本应答；实际 L3→L0→L1→L2→L3 路径与用户确认的长按退出、旋钮返回及 OLED 编号一致。下表五层、六槽 OLED 和旧迁移策略均属于 `0.2.0-qmk` 阶段的历史记录，最新验收目标以 `requirements.md` / `usage.md` / `development.md` 为准。

| 项目 | 当前处理 | 验证边界 |
|---|---|---|
| F01 | AG 输入不依赖宿主握手/任务灯分配，保留成对松开事件 | 主机回归通过；已刷入，实体 AG 输入待验收 |
| F02 | 按 ChibiOS 每份 USB 缓冲区分派，32/63/64 字节不合并 | 连续报告测试通过；实机 VIA 待验收 |
| F03 | 按用户澄清：普通层由 VIA 动作切到 0–4，不强占普通层长按键；新默认 L3 旋钮 B 为 `TO(4)`，不覆盖旧 EEPROM | 五秒离开 L4、静态/动态键位路由通过；旧 VIA 配置需自行保留返回键 |
| F04 | 旋转仅按下发送一次，按压仍发送按下/松开 | 实际按键处理函数测试通过 |
| F05 | 全灭任务灯不关闭本地 ACT 反馈 | 灯光测试通过；已包含在首刷灯光版 |
| F06 | 控制 SET_REPORT 入队主循环处理，满队列拒绝请求，兼容带/不带 ID | 控制校验、队列、实际收包函数和协议回归通过；实机控制路径待验收 |
| F07 | 单独 HID 查询与 configuration 选择相同模式的 Raw_HID | 实际描述符分支测试通过 |
| F08 | ambient/underglow 映射 ACT10/ACT11；keys 按用户要求不铺背景 | 灯光测试通过；MIC 实物快闪待验收 |
| F09 | KLE 使用相对偏移，恢复 16 键和三旋钮坐标 | 坐标累加检查通过；网页加载待验收 |
| F10 | 普通层显示层号，L4 显示六槽灯色标签，变化才重绘 | OLED 桩测试通过；不是任务标题/真实进度 |
| F11 | QMK L4 映射宿主层 1，普通 L0–3 映射 2–5，版本统一 | 五层状态和唯一请求 ID 测试通过 |
| F12 | 数字/字符串灯效、速度、原色、Idle 降亮 | 灯光测试通过；首刷 Idle 10% 后用户要求改为 5% |

其他边界已处理：拒绝 VIA 写入/读取 L4 地址和过长 buffer，但允许普通层存储 `TO(4)`；只有有效 VIA 命令锁定窗口；进入 L4 临时恢复 RGB 渲染，离开后恢复普通层开关/效果；安装脚本拒绝非固定 QMK commit。九组灯光、八组可靠性和一项 VIA 坐标检查共 18 项通过（ASan/UBSan），QMK 构建通过。所有带桩的离线结果均不代替实机验证，构建信息见 `development.md`。

## 对照来源

| 来源 | 本次核对版本 | 用途 |
|---|---|---|
| [hcyniubi/doio-kb16-codex-micro](https://github.com/hcyniubi/doio-kb16-codex-micro/tree/635bfe709b2ca399da6655bc87bc492ed12f8976) | `635bfe709b2ca399da6655bc87bc492ed12f8976` | QMK 协议实现和 PWM 驱动来源 |
| [conol-ai/openmicrokbd](https://github.com/conol-ai/openmicrokbd/tree/a668e11cfa8a7a3a24db6d13c85b75cad665d84a) | `a668e11cfa8a7a3a24db6d13c85b75cad665d84a` | Codex 报文、事件、控制传输、灯光配置 |
| [thompson-vii/kb16_rev2_vial_fw](https://github.com/thompson-vii/kb16_rev2_vial_fw/tree/3734492cac1a6c19316d2d2888d08898fd074977) | `3734492cac1a6c19316d2d2888d08898fd074977` | KB16 Rev2 引脚、矩阵、编码器、LED 编号 |
| [makecindy/cindy](https://github.com/makecindy/cindy/tree/1dbff37a891f3d4c660f326084876e09598d80bf) | `1dbff37a891f3d4c660f326084876e09598d80bf` | 接收端如何处理 AG 输入、空任务槽和任务灯 |
| [QMK](https://github.com/qmk/qmk_firmware/tree/f5db962110615fd263c3ebf9f0402e7c2b08fbe4) | `f5db962110615fd263c3ebf9f0402e7c2b08fbe4` | 实际编译基线，ChibiOS USB 队列和编码器事件 |

OpenMicroKbd 的硬件并非 KB16；这里只用它核对协议，KB16 接线以 QMK/Vial 的 Rev2 定义交叉核对。上游有相同代码不代表该行为已经满足本项目需求。

## 优先处理的问题

### F01 · P1 · AG 输入依赖灯光同步，会吞掉有效按键

位置：`firmware/keymaps/codex_micro_via_v1/codex_micro_protocol.c:511`，尤其是 `host_seen` 和 `thread_lights[slot].assigned` 两个门槛。

即使 `sys.version` 已成功交互，只要没有收到使对应灯处于 assigned 状态的 `thstatus`，按 AG 就只显示 `EMPTY SLOT`，不发送 `v.oai.hid`。灯光同步失败因此会连带造成“AG 按了没反应”。收到 `e:"off"` 又会把 assigned 清掉，灯是否亮不能代表该键是否应当发送输入。

离线复现：握手成功后按 AG00，发送报告数为 0；给槽 0 下发亮灯状态后，同一个按键能发送按下和松开事件。

这是从 hcyniubi 继承的限制。[OpenMicroKbd 的事件编码](https://github.com/conol-ai/openmicrokbd/blob/a668e11cfa8a7a3a24db6d13c85b75cad665d84a/fw/src/codex/wire.rs#L1308)不以灯光作为 AG 输入条件。[Cindy 的空槽处理](https://github.com/makecindy/cindy/blob/1dbff37a891f3d4c660f326084876e09598d80bf/apps/desktop/src/main/worklouder-codex/WorkLouderCodexLightingController.ts#L402)还允许按空任务槽新建任务，当前固件会提前拦截该操作。

修复方向：让 AG 输入独立于任务灯，任务是否为空交给宿主判断。

### F02 · P1 · VIA 连续报文可能合并，随后丢失响应

位置：`firmware/qmk-core-patches/tmk_core/protocol/chibios/usb_main.c:535`。

VIA 模式的报告是 32 字节，但 `raw_hid_task()` 每次从字节流读取最多 64 字节。ChibiOS 的 `ibqReadTimeout()` 可以跨队列缓冲区取字节。如果此时已有两份 32 字节报告，会读成一份 64 字节数据，VIA 只解析第一条；回复时 `send_raw_hid()` 又因当前模式只接受 32 字节而直接返回。

离线复现：使用实际 `raw_hid_task()` 函数和保持相同字节流语义的接收桩，单份 32 字节报告能回复；两份已排队的报告只产生一次 64 字节分派，回复数为 0。它证明的是排队条件下的固件缺陷，不代表所有 VIA 请求都会触发。

修复方向：按当前身份的报告长度读取，并保持每份 HID 报告边界。

### F03 · 按用户澄清处理 · VIA 层动作可以返回 L4

位置：`firmware/keymaps/codex_micro_via_v1/keymap.c:48`、`:72`、`:160`。

`LAYER_CYCLE` 只存在于 Layer 4 的 R4C1。长按 5 秒进入 Layer 0 后，该位置变成 `MT(MOD_LGUI, KC_MUTE)`；Layer 1–3 同一位置透明，回落到 Layer 0。因此默认配置不能实现约定的 `L4→L0→L1→L2→L3→L4`。

此前建议在所有普通层统一占用长按键并不符合用户最终要求，已撤回。VIA 仅限制可编辑的键位地址为 L0–L3，层动作目标仍可为 L0–L4；L4 即 Codex。普通层自行配置 `TO(4)`，新默认 L3 旋钮 B 提供该返回动作，旧 EEPROM 不重置。L4 左下角长按 5 秒只负责进入 L0。

### F04 · P2 · 旋钮 C 每格发送两次事件

位置：`firmware/keymaps/codex_micro_via_v1/keymap.c:213`。

QMK 的 encoder map 每格产生一次按下、一次松开。当前 `ENC_CC` / `ENC_CW` 在两次回调中都发送 `act:2`，会让滚动、调节等动作翻倍。上游的独立旋转处理没有这个重复路径。

离线复现：对实际 `process_record_user()` 输入一对旋转按下/松开，捕获到两份 `ENC_CW` 事件。

修复方向：旋转只在按下事件发送一次；编码器按压仍保留按下和松开。

### F05 · P2 · 六个任务灯全灭后，ACT 按下反馈也被关闭

位置：`firmware/keymaps/codex_micro_via_v1/codex_micro_protocol.c:343`、`:672`。

完整的六槽熄灯状态会设置 `lighting_off=true`。RGB 渲染在绘制 ACT 按下反馈前直接返回，所以 ACT 不再满足“平时不亮、按住才亮”。

离线复现：初始化后按 ACT06 能点亮 LED 8；收到六个 `e:"off"` 后再按 ACT06，LED 8 保持黑色。

修复方向：任务灯熄灭与 ACT 按住反馈分开处理。

## 其他明确缺口

### F06 · P2 · 缺少 Raw HID 控制写入和报告体归一化

位置：`firmware/qmk-core-patches/tmk_core/protocol/chibios/usb_main.c:274`；`firmware/keymaps/codex_micro_via_v1/poc_via.c:16`；`codex_micro_protocol.c:720`。

`HID_REQ_SetReport` 只处理键盘/共享接口的键盘灯，未处理 Raw HID 接口。Codex 路由又只接受包含 Report ID 的完整 64 字节报告，会拒绝已剥离 ID 的 63 字节报告体。后者已离线复现。

[OpenMicroKbd](https://github.com/conol-ai/openmicrokbd/blob/a668e11cfa8a7a3a24db6d13c85b75cad665d84a/fw/src/codex/mod.rs#L444)同时接收中断 OUT 和控制 SET_REPORT，其重组器接受带/不带 ID 的报告。当前差异是兼容性缺口；尚未抓取这台设备的 USB 包，不能断言此前的 10 秒超时就是由此造成。

修复时应先在安全缓冲区收控制报告，再在主循环分派，避免在 USB 中断里直接解析 JSON、写 EEPROM 或发送阻塞响应。

### F07 · P2 · VIA 的两种 HID 描述符查询返回不一致

位置：`firmware/qmk-core-patches/tmk_core/protocol/usb_descriptor.c:1287`、`:1412`。

VIA configuration 中的 Raw HID 报告描述符长度已改为 VIA 的 34 字节；单独查询 `HID_DTYPE_HID` 时却仍返回 Codex 那份，宣称长度为 36 字节。两个长度已从当前 ELF 符号确认。是否导致枚举失败取决于宿主查询路径，但描述符应保持一致。

### F08 · P2 · rgbcfg / preview 只应答，没有应用灯光配置

位置：`firmware/keymaps/codex_micro_via_v1/codex_micro_protocol.c:427`。

`v.oai.rgbcfg` 和 `lights.preview` 直接返回 `ok:true`，没有解析或保存参数。任务灯依靠另外的 `thstatus` 更新，不能据此声称宿主灯光配置已经完整兼容。这是继承自 hcyniubi 的缺口；OpenMicroKbd 实现了相应状态更新。

修复时要保留用户确定的 ACT 按住反馈、空位默认灭灯规则，明确宿主亮度/预览参数对这些规则的作用。

### F09 · P2 · VIA 布局把绝对坐标写成 KLE 相对偏移

位置：`firmware/via/kb16_codex_via.json:15`。

QMK `info.json` 的 x/y 是绝对坐标，VIA 的 KLE 属性是相对偏移，不能直接照抄。按 KLE 累加后，四排首键的 y 实际为 `0 / 2 / 5 / 11.5`，不是 `0 / 1 / 2 / 3`。旋钮 A 的 x 成为 `8.75`，不是 `4.75`；旋钮 B/C 也错位。JSON 能解析不代表布局正确。

### F10 · P2 · OLED 尚未实现所需层号和任务状态

位置：`firmware/keymaps/codex_micro_via_v1/oled_icons.c:172`、`:181`。

常驻画面只有旧的四张静态图，Layer 0 和 Layer 4 都走同一个 Codex 图标，没有常驻数字层号。任务状态只更新灯光数组，没有进入 OLED 渲染；`CONNECTED` / `TASK N REQUESTED` 弹窗不能代替六槽运行状态。

### F11 · P2 · device.status 总是报告同一层

位置：`firmware/keymaps/codex_micro_via_v1/codex_micro_protocol.c:423`。

`layer_index` 写死为 1，无论处于 Layer 0 还是 Layer 4 都不变。应定义宿主协议层号与 QMK 层号的对应关系后，返回实际状态。有线设备使用 `battery:100,is_charging:false` 与参考实现一致，不能把“功能受限”解释为 KB16 没有电池。

### F12 · P2 · 数字灯效未解析，宿主的呼吸请求变成常亮

后续颜色专项核对补充：本机 ChatGPT `26.1023.51421` 使用 Work Louder SDK 的数字枚举，`sendThreadsLighting()` 原样发送 `e:4` 表示呼吸；当前 `update_thread_object()` 却仅通过 `copy_json_string()` 读取效果。

离线复现：同一槽下发蓝色、满亮度，在计时为 0 时，`e:4` 得到 RGB `(26,72,171)`（常亮）；`e:"breath"` 得到 `(9,25,60)`（呼吸最低亮度）。探针使用实际协议源码，开启 AddressSanitizer / UndefinedBehaviorSanitizer。该缺陷从 hcyniubi 继承，OpenMicroKbd 兼容数字和字符串效果。

状态色还会经过固件的自定义改色表，白色额外替换为偏紫白；完整色值和灯效范围见 `protocol.md`。本次仅记录和离线验证，未修改固件行为。

## 构建和需求边界（修复前记录）

- VIA 目前只隐藏第五层，没有在 `quantum/via.c` 的单键、编码器和 buffer 写入命令中拒绝 Layer 4 地址。静态 flash 读取覆盖使实际 Codex 键位不会被这些 EEPROM 写入改变，但“拒绝 VIA 写入 Layer 4”的协议要求尚未落实。
- Codex 指示灯仍依赖 QMK RGB Matrix 效果渲染入口。如果普通层通过 VIA 配置关闭 RGB，或选择不运行指示器的效果，进入 Codex 层时没有独立恢复任务灯渲染的逻辑；这与上述六槽全灭问题是两个不同条件。
- `apply-core-patches.sh` 全文件覆盖 QMK，却不检查 QMK commit。当前基线编译通过，不代表任意后续 QMK 主线都兼容。
- 当前 keymap 没有自己的回归测试。旧 keymap 的测试明确针对旧层模型、Channel 3 和旧 ACT11 限制，不能用它们替代本次实现的测试。
- `docs/development.md` 等仍保留旧的旋钮分工、3 秒回层、中旋钮切层说明；需求文档也混有旧 Mic 位置和 `reset_keyboard()` 表述。验收说明需要跟实际 5 秒切层、A/B 摇杆和 C 编码器一致。

## 此前通信诊断的修正

截至本次 11:37（Asia/Shanghai）的日志取样，当前连接已枚举为 `303A:8360`。11:31:25 起出现 **53 次 `Received answer`**：19 次 `v.oai.rgbcfg`、27 次 `v.oai.thstatus`、7 次 `device.status`；该段没有 `TIMEOUT`，另有 53 次 `No resolver found`。

同一 ID 在成功应答后又重复返回，与先前已刷版本的延迟/重发逻辑相符；本次没有读取实机 flash 来确认镜像哈希。当前待刷源码已去掉重发，不能把本地新源码等同于设备里正在运行的版本。现有日志足以证明双向 RPC 曾经且目前能够成功，不足以保证每个物理按键和 LED 都正确。

此前把问题直接定性为“App 收包问题”或“QMK 默认四缓冲静默丢包”证据不足。检查当前 ChibiOS 接收实现可见，队列满时停止安排下一次 OUT 接收，释放缓冲区后再继续，具备背压逻辑。增大 `RAW_OUT_CAPACITY` 可以改变排队能力，但尚不能证明修复了之前的延迟根因；`rules.mk` 里的确定性归因注释应更正。

## 已通过的检查（最初审计构建）

- 当前仓库的 6 份 core C/H patch、16 份 keymap C/H/MK 文件与实际 QMK 构建树逐字节一致。
- `qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1` 成功。
- 镜像主体 48,444 字节；带 DFU suffix 的 `.bin` 为 48,460 字节；SHA-256 为 `122e4e0d6f1fed0ca6c2ab55a1188ea2d8483e2f31175856d42e86bcf8877d1c`。这是本地待刷镜像，不是实机镜像验证结果。
- `.data` 1,220 字节、`.bss` 6,792 字节，另预留栈 3,072 字节，剩余约 9 KB 堆空间。`size` 汇总会把预留堆算进 BSS，不能据此判定 SRAM 已耗尽。
- KB16 Rev2 的行列引脚、COL2ROW、三个编码器引脚、OLED I2C2 和 16 颗 LED 行优先编号与 QMK/Vial 定义一致；上游的 WS2812 DMA 事件补丁已包含在当前仓库。没有发现新的接线定义差异，物理方向仍以实机验收为准。
- 临时离线探针使用当前协议源码及实际提取的事件/收包函数，复现 F01、F02、F04、F05、F06 的短报告拒绝和 F11；协议与编码器探针开启 AddressSanitizer / UndefinedBehaviorSanitizer，没有报告内存错误。KLE 坐标另外按相对偏移计算核对。

上述原始建议现已按顶部状态表处理；此前 10 秒延迟若再次出现，应结合唯一请求 ID 和实际 USB 包定位，避免凭重复 ID、延迟重发或软件队列容量推断根因。灯光版首刷的四项 RPC 当时不能证明后续修复已运行；之后已另行刷入修复版并取得实机 `0.2.0-qmk` 应答，分阶段目视结果见 `lighting.md`，不把它扩展为所有功能均已通过。
