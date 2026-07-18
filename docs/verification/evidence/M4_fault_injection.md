# M4 验收证据：systemd 与故障注入

## 状态

Planned。M3 未完成前不进入 M4 正式验收。

## 计划验收

| 场景 | 预期 | 结果 |
| --- | --- | --- |
| `kill -9` | systemd 按策略恢复服务 | Pending |
| reboot | 服务自动启动 | Pending |
| camera missing | 服务可诊断地 degraded | Pending |
| camera restored | 按设计恢复采集 | Pending |
| bad config | 明确日志并 fail safe | Pending |
| `journalctl` | 可追踪启动、失败和恢复 | Pending |

M4 实现后补充 service 安装命令、unit 状态、日志、重启时间和关闭条件。正式运行中产生实际影响的事故进入 [`operations/postmortems/`](../../operations/postmortems/)，开发调试问题仍进入 [`bug_reports/`](../../bug_reports/)。
