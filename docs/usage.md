# 使用与刷写

## 当前 QMK + VIA 固件（主实现）

开发版 `0.6.0-qmk`：Codex 在第一页 L0，普通层在 L1–L3；五处直接读取 VIA，取消固定长按退出。L0 配 MO(1) 按住临时普通 L1、松开返回；普通层 TO(0) 回 Codex。原配置及层动作一次迁移，先导出备份。本版未发布/刷入；刷写仅使用对应提交的 main 流水线 Artifact，设备仍是 0.5.0；[完整说明](codex-via-keys.md)。**下方 0.4.9 下载与实机验收段为旧版本示例，不是当前源码的层序/退出方式。**

已发布/实机基线为 `0.4.9-qmk`：OLED 独立息屏，无任务灯分配保留 Logo，入层完整展示约 1 秒；背光、10 秒 VIA 窗口、左下角一秒退出、四层和 EEPROM 不变。流水线版本已刷入并核验，用户确认返回 Codex 的 Logo 正常；一小时息屏及按键唤醒按用户选择不测，不能记为实机通过，见 `development.md`。十三槽仍需宿主配套。

- 上电 10 秒内收到有效 VIA 命令，就锁定 VIA 身份（`D010:1601`）直到断电。VIA 显示四页，开发版 L0 为原生 Codex 加五处自定义，L1–L3 普通改键，等待/锁定初始层为 L1。
- 10 秒内没有 VIA 命令，键盘自动复位成 Codex 身份（`303A:8360`），开发版默认 L0，即 Codex 原生层。
- 断电重插 = 重新给一次 10 秒 VIA 窗口。

详细设计见 `via-dual-mode.md`，构建见 `development.md`。

## 前置条件

- DOIO KB16-01 **Rev2**（APM32F103CBT6）。
- macOS 或 Windows；刷写需要 `dfu-util` 或 QMK Toolbox。
- ChatGPT/Codex 桌面版（macOS 14+ / Windows 10+）。

## 0.6.0 流水线固件刷写

此流程适用于当前 L0 Codex / L1–L3 普通 VIA 的 `0.6.0-qmk`，不使用下方旧 Release 的二进制。提交到 main 自动构建，**没有创建版本标签或 Release**；下载 Actions 中与目标提交 SHA 完全对应的成功运行，在 Artifacts 获取 `firmware-0.6.0-qmk-<提交前12位>` 并解压。

1. **升级前导出 VIA 配置并保留原厂回退镜像**；升级会将旧 L3→新 L0，旧 L0/L1/L2→新 L1/L2/L3，包括层动作目标；旧 Codex MO(0) 变为 MO(1)。不要直接导入未转换的旧页序备份。迁移期间保持供电。
2. Artifact 应有 BIN、HEX、VIA JSON、`BUILD-INFO.json`、`SHA256SUMS` 五份附件。检查 `BUILD-INFO.json` 的源码 SHA 与当前成功运行一致，并在解压目录运行：

```bash
shasum -a 256 -c SHA256SUMS
```

3. **拔下 USB 后拆机**，露出 PCB 内部 `RESET`，放在绝缘表面。接回 USB，先启动下面的等待命令，再短按内部 RESET 并松开。目标必须为 Maple DFU `1EAF:0003`、alt 2 `Upload to Flash 0x8002000`，只写应用区，不动 Bootloader/读保护：

```bash
dfu-util -w -a 2 -d 1EAF:0003 \
  -D doio_kb16_rev2_codex_micro_v0.6.0-qmk.bin
```

4. 下载期间不拔线、不再复位；出现 `UNKNOWN`/`LIBUSB_ERROR_PIPE` 先停下，断电重新进入，不做全片擦除。100%/`Download done.` 后复位返回 `74` 时先检查重新枚举和版本，不因此盲目重刷。HEX 不能作为原始 BIN 传入。
5. 真正断电等 5 秒，正常插回，**不按 RESET、不连 VIA**：应看到 `VIA WAITTING` 约十秒，然后 Codex L0 的 Logo/任务页。新版本及实际键位/屏幕需分别验收；只成功下载不等于所有功能通过。
6. 改键时在下一次冷启动十秒内连接 VIA 并成功读取，身份会锁定到断电。第一页面 L0 的五处可自配；左下可设 `MO(1)`，按住普通 L1、松开 Codex；普通层可配 `TO(0)` 返回。全新五处默认无动作，无固定长按退出；页面 L1/L2/L3 空闲显示 2/3/4。改完断电，让窗口超时才恢复 Codex USB 身份。

本轮仅测试、提交推送和给出方法，不执行上述硬件刷写。正式来源与旧版恢复记录见 [开发](development.md)。

## 历史 Release 0.4.9 的刷写示例

### 1. 构建或获取固件

Git 不存放预编译固件；正式刷写从 [Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases/tag/v0.4.9-qmk) 下载流水线生成的 BIN/HEX、VIA 定义、BUILD-INFO.json 及 SHA256SUMS。核验源码提交和流水线链接，并在下载目录执行 `shasum -a 256 -c SHA256SUMS`。本地构建仅供开发验证，不替代正式发布附件：

```bash
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```

产物是 `doio_kb16_rev2_codex_micro_via_v1.bin`（当前约 51 KB）。刷前先导出 VIA 配置，核对当前产物 SHA-256；不要将地址兼容理解为任意升级都不重置配置。

### 2. 进入 Bootloader

本项目这台 Rev2 需**拆机按 PCB 内部 `RESET`**。先断开 USB，再拆外壳，把 PCB 放在绝缘表面，避免金属工具短路；不要操作 BOOT/SWD 焊盘、擦 Bootloader 或修改读保护。

接回 USB，先在电脑启动下面的等待刷写命令，再短按内部 `RESET` 并松开，以捕获短暂的 DFU 窗口。旧日志中的 RET 叫法不代表外壳外有可按的复位键；以内部实物为准，普通键或软件切层不能替代。也可用 `dfu-util -l` 先确认设备：

```text
[1eaf:0003] STM32duino bootloader
```

### 3. 刷写

```bash
dfu-util -w -a 2 -d 1EAF:0003 -D doio_kb16_rev2_codex_micro_v0.4.9-qmk.bin
```

alt 2 必须显示 `Upload to Flash 0x8002000`（应用起址 `0x08002000`）。只使用 BIN；HEX 不能当作原始二进制传给此命令。确保只有目标这台 DFU 设备，下载期间保持 USB 连接，不再按 RESET。

BIN 大小以本版 `BUILD-INFO.json` 为准，下载主体不含文件末尾的 16 字节 DFU suffix。若显示 100% / `Download done.` 后复位返回 `74`，先检查设备是否重新枚举、运行版本是否为 `0.4.9-qmk`，不要自动重复写入。若出现 `UNKNOWN` / `LIBUSB_ERROR_PIPE`，停止并断电，重新用内部 RESET 进入 DFU；不要用全片擦除、解保护或改 Bootloader 来试错。

### 4. 旧 Release 0.4.9 的正常启动与切层验收

刷完后拔 USB 等 5 秒，再正常插回：**不按 RET、内部 RESET 或任何其他键，也不开 VIA**。前 10 秒为 `D010:1601`，OLED 应显示 `VIA WAITTING`；未收到 VIA 命令时自动复位成 `303A:8360`，进入 Codex L3，先 Logo+CODEX，再任务页。

先短按左下角（不足 1 秒），应留在 Codex；再按住约 1 秒，应进入普通 L0，OLED 显示 `1`。通过已配置的 `TO(3)` 返回 Codex；默认布局可按右上旋钮 B 三次经 L1/L2 回 L3，已有配置以自己保存的键位为准。

USB 从 VIA 枚举到 Codex 的时间只是启动窗口的近似观测，不等于精确量到通电时刻或看到了 OLED。屏幕原文、约 10 秒显示及手按约 1 秒切层需逐项目视确认。刷完的自动复位可能保留 Codex 标志，不能代替真正断电重插测试。

### 5. 旧 Release 0.4.9 的 Logo 与一小时息屏验收

1. 先核验 `sys.version` / `device.status` 返回刷入版本；状态查询只能证明版本/层号，不能证明屏幕已经显示。
2. 长按左下角约 1 秒退出 L3，再通过 `TO(3)` 返回。目视确认先出现 Logo+CODEX 约 1 秒；随后有任务灯分配则进入状态页，没有任务灯分配则保留 Logo。无任务是未分配/明确 off，不是 B=0 或瞬时呼吸变暗。
3. 真正的一小时息屏测试需要保持 USB 正常供电、电脑不挂起，不触碰 KB16，并暂停该键盘的宿主通信。**有效心跳、重复灯态、版本/状态查询都会续时**；不要一边轮询状态一边等息屏。若临时测试工具独占专用接口，应停止一切写请求，并说明 Codex 通信会暂时不可用；测试后关闭工具释放接口，不改宿主持久配置。
4. 自最后一次输入/有效报文起静置至少一小时，先目视确认 OLED 熄灭，再按一个实体键。建议在 Codex 使用左上空位，避免触发任务动作；其他有功能的键在唤醒时也照常执行，不吞首次按键。确认屏幕恢复后再恢复宿主通信，避免把心跳唤醒误认成按键唤醒。
5. USB 挂起造成的熄屏不是一小时空闲测试；下发 RGB off/B=0 也不等于 OLED 息屏。不要换短超时固件后声称正式 Release 的一小时实机验证通过。

本次 `0.4.9-qmk` 刷写：流水线 BIN 50,880 字节，DFU 应用主体 50,864 字节一次下载完成；复位返回 `74` 后读取版本确认成功，未重复刷写。主机捕获 L3→L0→L1→L2→L3，用户回复“Logo 显示正常”。**用户选择不做一小时测试，因此息屏/按键唤醒仍仅有代码与离线测试覆盖**。未启动一小时独占监测、未注入模拟灯光，详细日志/哈希见 `development.md`。

## ChatGPT / Codex 设置

打开 ChatGPT/Codex 桌面版 → **Settings → Codex Micro**（设备连着才会出现；部分版本在 Settings → Keyboard shortcuts → Accessories）。

可设置：

- **Agent keys（AG00-AG05）**：Pinned / Recent / Priority / Custom；每个键指定 chat、keycap、shortcut 或 skill；单击 vs 双击聚焦。
- **Action keys / Keycaps（ACT06-ACT12、Mic）**：命令、技能、自定义快捷键、插入文本；换 keycap。
- **Analog stick**：四个方向各绑一个命令或技能。
- **Knob**：composer navigation / reasoning / conversation scroll / custom；左转、右转、单击、长按。
- **Microphone key**：push-to-talk 或 Voice Chat；是否拆成两个麦克风键。
- **Brightness**：所有设备灯光亮度。
- **Connection / Battery**：连接与电量；Reset keyboard layout 恢复默认。

> 这些是宿主侧原生功能绑定，具体项目以当前 App 为准，不是 VIA 物理改键。本仓库只生成固件、不包含 Mapper；三个普通层执行 VIA 配置，0.6.0 在第一层 L0 开放指定五处，其余原生 Codex 控制固定。6 层/AppSense 不属于当前实现。

## VIA 配置与当前 0.6.0 切层

- 提前载入 VIA 定义，断电重插十秒内成功读键位即可锁定 VIA 身份直到断电。四页均保存；第一页 L0 仅五处执行，第二至第四页 L1–L3 普通动态执行。
- 第一页左下角配置 `MO(1)`，Any 为 `0x5221`：Codex 按住临时普通 L1，同时可按该层的其他键，松开回 Codex。左上同样可配这种持续层动作，无固定长按。
- 普通层用 `TO(0)`（Any `0x5200`）回 Codex；`TO(1/2/3)` 常驻普通层。标准 QMK 高层优先，向低层 MO 不是强制降层。
- 五处全新默认无动作，请自行配置切层入口。普通层默认 B 按压 L1→L2→L3→L0；已有用户配置迁移保留。
- 旧 L3→新 L0，旧 L0/L1/L2→新 L1/L2/L3，层动作目标同步变化，原 Codex MO(0) 变为 MO(1)；五处编辑/宏不再清空，地址不动。
- 导入旧顺序备份前先转换页序及目标；迁移不是断电事务，升级时保持供电并备份，完整边界见 [改键文档](codex-via-keys.md)。
- 改完断电重插，关闭 VIA，让十秒窗口超时进入 Codex USB 身份默认 L0。切层不改 USB 身份，锁定 VIA 后仅切 L0 无法供 Codex 软件使用。
- 新版实机待验收：十秒窗口、MO(1) 及跨层释放、TO(0)、无固定长按、普通页 2/3/4、Codex Logo/任务/实际输出；当前不自动刷机。

## OLED

`0.4.9-qmk` 不再因宿主任务/环境灯全灭或零亮度而关屏；无任务灯分配持续显示 Logo+CODEX，有任务则入层展示约 1 秒 Logo 后转六格任务页。仅 USB 挂起或一小时无输入且无有效通信关闭 OLED；心跳/重复有效报文续时。正常冷启动不按 RET/内部 RESET，才可观察完整 `VIA WAITTING` 约 10 秒流程；画面与时长不能以 RPC 应答代替验收，详见 `oled.md`。

## 恢复官方 VIA 固件

如果刷坏了或想回到 VIA：

1. 断开 USB 后拆机，接回 USB 并启动刷写等待，再按 PCB 内部 `RESET` 进入 Maple DFU；仍只写 alt 2 应用区。
2. 刷入官方 Rev2 VIA 3.0 固件（例如 `2351080960doio-kb16_via（刷3.0）.bin`）。
3. 如果 VIA 读不到键位，先导入匹配的 VIA 定义；必要时恢复应用配置默认值，不要擦除 Bootloader 或修改读保护。

官方 VIA 3.0 固件 SHA-256（当前手头文件）：

```text
0d1f21580b81d88f0e9e4a1bb87ae5a7a96f72677d12864651b6a0624cccac01
```

## macOS 注意

- ChatGPT/Codex 桌面版支持 macOS，并内置 Codex Micro 识别。
- 本仓库不含 Mapper；改键用浏览器版 VIA 即可。
- 刷写可用 `dfu-util`；进入 Bootloader 后按上面的命令执行。
