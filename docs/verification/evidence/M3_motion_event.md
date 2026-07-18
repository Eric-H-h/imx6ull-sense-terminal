# M3 验收证据：Motion Event

## 状态

Planned。M2 未完成前不进入 M3 实现。

## 进入条件

- M2 浏览器流、状态接口和稳定性验收完成。
- 已确定适合当前 MJPEG 架构的 motion 输入方案。

## 计划验收

| 场景 | 预期 | 结果 |
| --- | --- | --- |
| 静止画面 | 不持续触发事件 | Pending |
| 明显挥手或物体移动 | 触发 motion event | Pending |
| cooldown 内重复变化 | 不连续刷写事件 | Pending |
| JSONL 格式 | 每行可独立解析 | Pending |
| `/status` | motion 和 event count 为真实状态 | Pending |

实际实现后补充命令、阈值、运行环境、输出和调参结论。
