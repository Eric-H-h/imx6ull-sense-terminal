# M4：systemd 与故障注入

## 阶段状态

- 状态：Completed
- 完成日期：2026-08-20
- 分支：`codex/m4-systemd-fault`
- Commit：`9df25c5`（功能）、`6f631f6`（文档）；PR #5 合入 `develop` 为 `bee388a`

## 阶段目标

把已验收的 M3 daemon 变成可无人值守运行的 systemd 服务：正式安装路径、低权限运行、开机自启，以及 kill、reboot、摄像头缺失、非法配置和事件日志写失败的可诊断恢复。

## 验收结果

| 验收项 | 结果 | 证据 |
| --- | --- | --- |
| 正式安装路径与权限 | Passed | D2-B：binary/config/unit/`/var/lib` 属主和 mode |
| `systemd-analyze verify` | Passed | systemd 241，exit 0 |
| `enable --now` 后运行 | Passed | `enabled` / `active`，`debian:debian` |
| 摄像头缺失 degraded | Passed | 服务不退出，`NRestarts=0` |
| 摄像头插回同进程恢复 | Passed | 原 PID 采集恢复 |
| SIGTERM 正常退出 | Passed | `Result=success`，不误触发重启 |
| `kill -9` 自动恢复 | Passed | D3-A：新 PID，`NRestarts=1` |
| 非法配置 fail-safe | Passed | D3-B：退出码 78，12 秒无重启风暴 |
| 事件日志写失败 | Passed | D3-C：推流继续，`event_log_state=unavailable` |
| reboot 自启与 JSONL 持久化 | Passed | D3-D：`who` 为空即已运行，前 15 行 sha256 不变 |

完整命令和输出见 [M4 evidence](../verification/evidence/M4_fault_injection.md)。操作入口见 [服务生命周期 runbook](../operations/runbooks/service-lifecycle.md)。

## 板端与运行事实

- 开发板/内核：EBF6ULL S1 Pro，Linux 4.19.35-imx6，ARMv7，systemd 241。
- 网络路径：USB RNDIS，板端 `192.168.7.2`。不要启用 `autowifi.service`。
- 二进制：`/usr/local/bin/imx6ull-sense`
- 配置：`/etc/imx6ull-sense/config.json`
- 工作目录与事件日志：`/var/lib/imx6ull-sense/events.jsonl`
- unit：`/etc/systemd/system/imx6ull-sense.service`
- 运行用户：`debian:debian`
- 重启策略：`Restart=on-failure`，`RestartSec=3`，`RestartPreventExitStatus=78`
- 摄像头：动态识别 UVC Capture 节点；reboot 后节点号可以从 `/dev/video2` 变为 `/dev/video1`

## 关键变化

- 代码：配置错误退出码 78；event log 写失败进入 `/status` degraded，不停止 HTTP 与采集。
- 构建：ARM 产物由 `scripts/build-arm.sh` 提供；安装不擅自 enable/start。
- 部署：新增 `scripts/install-service.sh` 与 `scripts/verify-service-package.sh`；删除占位 `scripts/deploy_placeholder.sh`。
- 配置：正式 `event_log` 为工作目录相对路径 `events.jsonl`。
- 文档：M4 evidence、`M4-DRV-WIFI-001`、服务 runbook 和本阶段总结。

## 学习型 Bug / Blocker

- [M4-DRV-WIFI-001](../bug_reports/M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md)：AP6212 DHD/SDIO 打开路径可导致用户态失去响应。项目范围通过停用 `autowifi.service` 解除阻塞；底层 BSP 根因不纳入 M4。

## 实际运行事故

无。M4 仍是开发验收，没有正式长期运行后的事故。

## 未关闭风险

- `event_count` 是进程内计数，daemon 重启后归零；JSONL 在磁盘上继续追加。这是已确认语义，不是缺陷。
- JSONL 追加不 `fsync`；断电时最后一行持久性仍不保证。
- `event_log_state` 在写失败后保持 unavailable，直到下一次成功追加，不会因只恢复文件路径而自动变回 `ok`。
- 板端 RTC 不可靠，journal 墙钟可能错乱；验收以 monotonic、PID 和文件 sha256 为准。
- HTTP 仍无认证，只适用于可信局域网。
- AP6212/DHD 不适合作为本项目部署路径。

## 关键决策

- 延续 [ADR-0003：最小 daemon](../architecture/decisions/0003-build-minimal-daemon.md)：M4 不新增业务能力。
- 延续 [ADR-0004：摄像头失败 degraded](../architecture/decisions/0004-camera-failure-degraded-mode.md)：硬件缺失由 daemon 自恢复，不靠 systemd 重启。
- 配置错误用退出码 78 与 `RestartPreventExitStatus` 区分于崩溃恢复。
- 正式事件目录使用 `/var/lib/imx6ull-sense/`，而不是 `/var/log/`。

## 下一阶段进入条件

上述条件已满足：PR #5 已合入 `develop`，本地已 fast-forward 到 `bee388a`，并创建 `codex/m5-docs-demo`。M5 只做测试报告、Demo 和发布包装，不新增 daemon 功能。
