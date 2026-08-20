# M3：Motion Event 与 JSONL

## 阶段状态

- 状态：Completed
- 完成日期：2026-07-26
- 分支：`codex/m3-motion-event`
- Commit：`209cb25`（PR #4 已合入 `develop`）

## 阶段目标

在不破坏 M2 UVC MJPEG 浏览器推流的前提下，从 latest JPEG 低频抽样得到可解释 motion score，通过 threshold 与 cooldown 生成真实事件，追加 JSONL，并把 motion 状态暴露到 `/status`。

## 验收结果

| 验收项 | 结果 | 证据 |
| --- | --- | --- |
| 主机 clean build 与测试 | Passed | `make -C app/daemon verify`，8组测试通过 |
| ARM 交叉编译 | Passed | ARM EABI5，依赖板端 `libjpeg.so.62` |
| 静止误报 | Passed | 5 分钟 `static_delta=0` |
| 明显动作 | Passed | 10 次挥手识别 10 次 |
| cooldown | Passed | 持续动作产生 7 个受限事件 |
| JSONL | Passed | 13/13 行可独立解析，字段齐全 |
| `/status` | Passed | motion、score、采样 FPS、事件数均为真实值 |
| 摄像头拔插恢复 | Passed | 同进程恢复，第一帧只重建 baseline |
| 浏览器并行 | Passed | client 1、stream 30 FPS、motion 3 FPS |
| 30 分钟稳定性 | Passed | PID 不变、RSS delta 0、无新增相关内核错误 |

完整命令、参数和输出见 [M3 evidence](../verification/evidence/M3_motion_event.md)。

## 板端与运行事实

- 开发板/内核：EBF6ULL S1 Pro，Linux 4.19.35-imx6，ARMv7。
- 摄像头：动态识别 `uvcvideo + Video Capture + Streaming`，本次为 `/dev/video1`。
- 网络路径：USB RNDIS，板端 `192.168.7.2`。
- 视频：MJPG 640x480@30，浏览器 multipart pass-through。
- Motion：3 FPS，160x120 grayscale，像素差 25，变化比例 5%，cooldown 1500 ms。
- 事件路径：验收阶段使用 `/tmp/imx6ull-sense-m3/events.jsonl`。

## 关键变化

- 代码：新增 JPEG decoder、motion detector、event gate、JSONL event log、pipeline、worker 和 status JSON 模块。
- 状态：AppState 增加 capture generation、JPEG snapshot 和真实 motion 字段。
- 配置：新增 motion enable、sample FPS、scale、两层阈值、cooldown 和 event log。
- 构建：新增 `make verify` 与 `scripts/build-arm.sh`。
- 测试：新增 8 组模块与集成单元测试、JPEG benchmark、主机 analyzer 和板端稳定性验证。
- 文档：更新架构、原理、配置、HTTP API、evidence 和阶段总结。

## 学习型 Bug / Blocker

- [M3-ENV-PROXY-001](../bug_reports/M3-ENV-PROXY-001_sudo_apt_drops_proxy.md)：sudo 后 APT 丢失代理。
- [M3-OPS-SSH-001](../bug_reports/M3-OPS-SSH-001_ssh_consumes_piped_script_stdin.md)：SSH 消费编排脚本 stdin。
- [M3-OPS-WSL-002](../bug_reports/M3-OPS-WSL-002_powershell_wsl_runner_lifecycle.md)：PowerShell/WSL 换行与长期 runner 生命周期。

## 实际运行事故

无。M3 仍处开发和验收阶段，没有正式部署后的运行事故。

## 未关闭风险

- `event_count` 是进程内计数，daemon 重启后归零，而 JSONL 继续追加；M4 需决定跨重启语义。
- JSONL 每次事件追加并关闭文件，但不执行 `fsync`；断电时最后一条事件的持久性不是 M3 保证。
- event log 写入失败会写 stderr，但当前 `/status last_error` 仍主要描述采集错误。
- 阈值来自当前摄像头和室内场景，换设备或场景后需要重新校准。
- HTTP 仍无认证，只适用于可信局域网。

## 关键决策

- [ADR-0001：UVC first](../architecture/decisions/0001-use-uvc-first.md)
- [ADR-0002：MJPEG over HTTP](../architecture/decisions/0002-use-mjpeg-over-http.md)
- [ADR-0003：最小 daemon](../architecture/decisions/0003-build-minimal-daemon.md)
- [ADR-0006：延后 CameraSource seam](../architecture/decisions/0006-defer-camera-source-seam.md)

M3 延续单一 UVC/MJPEG adapter，没有提前实现 YUYV、OV5640 或插件 ABI。

## 下一阶段进入条件

1. 精确审查 M3 源码、文档和未跟踪文件范围。
2. M3 commit 与 PR 合入 `develop`。
3. 本地 fast-forward 到最新 `develop`，工作区干净。
4. 创建 `codex/m4-systemd-fault`。
5. M4 开始前确认正式二进制、配置、事件目录和 systemd 权限模型。
