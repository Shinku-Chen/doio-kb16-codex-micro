# PoC：USB 描述符切换（设计稿）

当前上游固件只有一个 Raw HID 接口：

- `FF00:0001`
- Report ID `6`
- 64 字节
- VID/PID `0x303A:0x8360`

VIA 需要：

- `FF60:0061`
- 32 字节
- VID/PID `0xD010:0x1601`

PoC 采用**顺序模式**，不做同时双接口：启动时根据模式选择一整套描述符。

## 描述符集

```c
typedef enum { POC_USB_DESC_VIA, POC_USB_DESC_CODEX } poc_usb_desc_t;

// VIA
#define VIA_VENDOR_ID 0xD010
#define VIA_PRODUCT_ID 0x1601
#define VIA_RAW_USAGE_PAGE 0xFF60
#define VIA_RAW_USAGE_ID 0x61
#define VIA_RAW_EPSIZE 32

// Codex
#define CODEX_VENDOR_ID 0x303A
#define CODEX_PRODUCT_ID 0x8360
#define CODEX_RAW_USAGE_PAGE 0xFF00
#define CODEX_RAW_USAGE_ID 0x01
#define CODEX_RAW_REPORT_ID 6
#define CODEX_RAW_EPSIZE 64
```

## 需要改的地方

1. `tmk_core/protocol/usb_descriptor.c`：
   - 准备两套 `DeviceDescriptor` / `ConfigurationDescriptor` / `RawReport`。
   - `USB_GetDescriptor()` 根据 `poc_usb_desc()` 返回对应描述符。
   - 端点大小：VIA 32B，Codex 64B；端点号可以复用。
2. `keyboard_init_user()` 里调用 `poc_mode_init()`：
   - 必须在 USB 枚举前读 `.noinit` / RTC 标志。
   - `poc_usb_select()` 设置当前描述符集。
3. 模式切换：
   - 写标志 → `reset_keyboard()`。
   - 启动时读到标志 → Codex 描述符；否则 VIA 描述符。

## 注意

- 顺序模式不需要同时暴露 FF00 和 FF60，因此不需要额外端点。
- 切换会重新枚举 USB；ChatGPT/VIA 都会短暂断开。
- 如果以后要做“同时双接口”，再考虑第二套 Raw HID 接口和端点预算。
