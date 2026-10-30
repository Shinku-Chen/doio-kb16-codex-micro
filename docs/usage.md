# 使用与刷写

## 当前 QMK + VIA 固件（主实现）

当前源码 `firmware/keymaps/codex_micro_via_v1` 为 `0.4.9-qmk`：OLED 独立息屏，无任务灯分配保留 Logo，入层完整展示约 1 秒；背光、10 秒 VIA 窗口、一秒退出、四层和 EEPROM 不变。流水线版本已刷入并核验，用户确认返回 Codex 的 Logo 正常；一小时息屏及按键唤醒按用户选择不测，不能记为实机通过，见 `development.md`。十三槽仍需宿主配套。

- 上电 10 秒内收到有效 VIA 命令，就锁定 VIA 身份（`D010:1601`）直到断电。VIA 显示四页，L0–L2 是普通层，L3 页面仅保存改键、不执行。
- 10 秒内没有 VIA 命令，键盘自动复位成 Codex 身份（`303A:8360`），默认 Layer 3，即 Codex 原生层。
- 断电重插 = 重新给一次 10 秒 VIA 窗口。

详细设计见 `via-dual-mode.md`，构建见 `development.md`。

## 前置条件

- DOIO KB16-01 **Rev2**（APM32F103CBT6）。
- macOS 或 Windows；刷写需要 `dfu-util` 或 QMK Toolbox。
- ChatGPT/Codex 桌面版（macOS 14+ / Windows 10+）。

## 刷入 Codex Micro 固件

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

### 4. 正常断电启动与切层验收

刷完后拔 USB 等 5 秒，再正常插回：**不按 RET、内部 RESET 或任何其他键，也不开 VIA**。前 10 秒为 `D010:1601`，OLED 应显示 `VIA WAITTING`；未收到 VIA 命令时自动复位成 `303A:8360`，进入 Codex L3，先 Logo+CODEX，再任务页。

先短按左下角（不足 1 秒），应留在 Codex；再按住约 1 秒，应进入普通 L0，OLED 显示 `1`。通过已配置的 `TO(3)` 返回 Codex；默认布局可按右上旋钮 B 三次经 L1/L2 回 L3，已有配置以自己保存的键位为准。

USB 从 VIA 枚举到 Codex 的时间只是启动窗口的近似观测，不等于精确量到通电时刻或看到了 OLED。屏幕原文、约 10 秒显示及手按约 1 秒切层需逐项目视确认。刷完的自动复位可能保留 Codex 标志，不能代替真正断电重插测试。

### 5. Logo 与一小时息屏后的按键唤醒

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

> 这些是宿主侧原生功能绑定，具体项目以当前 App 为准，不是 VIA 物理改键。本仓库只生成固件、不包含 Mapper；只有三个普通层执行 VIA 配置，第四层固定 Codex。6 层/AppSense 不属于当前实现。

## 改键/改灯

本仓库只负责生成固件，不包含 Mapper 或 Work Louder Input。

- 改 Layer 0–2：提前打开 <https://usevia.app> 并在 Design 加载 `firmware/via/kb16_codex_via.json`；断电重插后 10 秒内授权并成功读取键位，随后可持续编辑直到断电。
- VIA 第四页（Layer 3）可编辑、保存、读回，但不会执行这些改键；进入该层始终使用固件写死的 Codex 按键和旋钮。
- L3 左下角长按 1 秒进入 L0；在普通层配置 `TO(3)` 返回 Codex，Any 可填 QMK `0x5203`。
- AG 任务灯跟随 AG00–AG05 的物理位置，只在 Layer 3 生效；其它层显示普通 RGB 效果。

## 普通层

固件只定义四层：Layer 0–2 普通 VIA，Layer 3 固定 Codex，没有 Layer 4。

- L0–L2 默认保留原保存布局的前三层；旋钮 B 默认按压依次为 `TO(1)`、`TO(2)`、`TO(3)`。进入 L3 后该旋钮按压为空，仍用左下角长按退出。
- Layer 3：AG00–05、ACT06–12，旋钮 A/B 是摇杆 X/Y，旋钮 C 是编码器。
- 四层升级保留旧键位/旋钮/宏的 EEPROM 起址；一次把 L0–L2 按键及旋钮里目标为旧 L4 的层动作迁移到 L3。不重置其他键、第四页和宏，也不反复覆盖后续 VIA 修改；仍需刷前导出配置。
- 第四页原先普通 RGB 层的配置可保存，但不再执行。需要的普通功能请放到 L0–L2；后续导入旧布局时自行将返回目标设为 `TO(3)`。
- 临时回 Codex：正常拔插 USB，不按 RET、不连接 VIA，等 10 秒默认进入 L3。若锁定了 VIA USB 身份，只切层不会变成 Codex 身份；须断电后让窗口超时。

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
