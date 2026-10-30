# 使用与刷写

## 当前 QMK + VIA 固件（主实现）

当前源码 `firmware/keymaps/codex_micro_via_v1` 为 `0.4.8-qmk`：10 秒 VIA 窗口、Codex 长按 1 秒回 L0；60 项离线检查和编译通过，本版未刷机。上一版 `0.4.7-qmk` 已刷入并获用户正常反馈。休眠/灯光及六格 OLED 不变；十三槽仍需宿主配套。

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

Git 不存放预编译固件；可从 [Release](https://github.com/Shinku-Chen/doio-kb16-codex-micro/releases/tag/v0.4.8-qmk) 下载 BIN/HEX、VIA 定义及 SHA256SUMS，或按 `development.md` 本地构建：

```bash
export QMK_HOME=~/qmk_firmware
./firmware/apply-core-patches.sh
qmk compile -kb doio/kb16/rev2 -km codex_micro_via_v1
```

产物是 `doio_kb16_rev2_codex_micro_via_v1.bin`（当前约 51 KB）。刷前先导出 VIA 配置，核对当前产物 SHA-256；不要将地址兼容理解为任意升级都不重置配置。

### 2. 进入 Bootloader

拔掉 USB，按住背面 `RET` 键，再插上 USB。用 `dfu-util -l` 确认出现：

```text
[1eaf:0003] STM32duino bootloader
```

### 3. 刷写

```bash
dfu-util -a 2 -d 1EAF:0003 -D doio_kb16_rev2_codex_micro_via_v1.bin
```

刷完拔插一次：前 10 秒是 `D010:1601`（VIA 身份），10 秒内没有 VIA 连接就复位成 `303A:8360`（Codex Micro 身份）。

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

`0.4.7-qmk` 已刷并获用户正常反馈；当前 `0.4.8-qmk` 将冷启动等待改为 10 秒，本版未刷入。不按 RET、真正断电再上电，才能观察完整 `VIA WAITTING` 流程。留边、Codex Logo/任务页、普通数字/按键页和休眠规则不变；详见 `oled.md`。

## 恢复官方 VIA 固件

如果刷坏了或想回到 VIA：

1. 进入 Bootloader（按住背面 `RET` 键插 USB）。
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
