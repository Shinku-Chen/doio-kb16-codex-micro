# Codex 额度显示

## 结论

键盘固件本身不能联网，不能直接获取 Codex 额度。额度必须在 **电脑上的 host daemon** 里查询，再通过 Raw HID 发给键盘。

## 可用的数据源

Codex app-server 提供 JSON-RPC：

```text
account/rateLimits/read
```

响应包含 `rateLimits`（单桶）和 `rateLimitsByLimitId`（多桶）。关键字段：

```json
{
  "rateLimits": {
    "primary": {
      "usedPercent": 28,
      "windowDurationMins": 300,
      "resetsAt": 1793260000
    },
    "secondary": {
      "usedPercent": 55,
      "windowDurationMins": 10080,
      "resetsAt": 1793800000
    },
    "credits": {
      "hasCredits": true,
      "unlimited": false,
      "balance": "12"
    }
  }
}
```

- `primary` / `secondary` 用 `windowDurationMins` 区分：300 分钟 = 5 小时，10080 分钟 = 7 天。
- `usedPercent` 是已用百分比；显示剩余额度时用 `100 - usedPercent`。
- `credits.balance` 是字符串，可能为 `null`。

## daemon 伪代码

```text
1. 连接/启动 Codex app-server（stdio 或 unix socket）
2. 每 60s 调用 account/rateLimits/read
3. 找到 windowDurationMins=300 的窗口 → p5 = 100 - usedPercent
4. 找到 windowDurationMins=10080 的窗口 → p7 = 100 - usedPercent
5. 打开 KB16 Codex Raw HID（303A:8360, FF00/0001, report 6）
6. 发送 JSON-RPC：
   {"method":"v.oai.quota","params":{"p5":72,"p7":45,"cr":"12"},"id":1}
```

固件里的 `poc_quota_parse()` 会解析 `p5` / `p7` / `cr`，OLED 显示：

```text
L0 CODEX 5H 72%
1R 2W 3. 4U 5E 6-
7D 45% CR12
R run W wait U unread
```

## 注意

- `account/rateLimits/read` 是 Codex app-server 的内部接口，不是公开稳定 API；升级后字段可能变化。
- 5 小时/7 天窗口的映射要看 `windowDurationMins`，不要假设 `primary` 永远是 5H。
- 额度数据需要 host 登录 Codex；键盘单独使用时没有额度信息。
- 如果不想写 daemon，也可以先只显示层号和任务状态，额度留空。
