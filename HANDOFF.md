# HANDOFF — i.MX6ULL Sense Terminal

这份交接文档用于新的主线 Codex/Claude session 继续推进项目。

项目路径：

```text
WSL 路径:     /home/eric/projects/imx6ull-sense-terminal
Windows 路径: \\wsl.localhost\Ubuntu\home\eric\projects\imx6ull-sense-terminal
```

## 1. 当前目标

在野火 EBF6ULL S1 Pro / i.MX6ULL eMMC 开发板上完成一个小而完整的嵌入式 Linux 简历项目。

项目要能真实上板运行、能浏览器演示、能产生可复盘的日志和测试证据。不要扩展成大型视频监控平台，价值在于一个板端可验证、可解释、可靠的嵌入式 Linux 服务闭环。

MVP 目标链路：

```text
V4L2 摄像头 / UVC 采集
  -> JPEG/MJPEG 浏览器推流
  -> 简单 motion event 检测
  -> 本地事件日志
  -> systemd restart/watchdog 行为
  -> 故障注入文档
```

## 2. 当前仓库状态

仓库已在 WSL 路径下创建并完成 M0 初始化。这个仓库的 Git 操作必须优先在 WSL 内部执行；Windows Git 通过 UNC 路径访问 WSL 仓库时可能误判 modified 文件。

Git 状态：

- 当前工作分支：`codex/m1-uvc-capture`
- 集成分支：`develop`
- M0 初始化已提交。
- 当前已知基础提交：
  - `8408572 docs: add git workflow rules to execution plan`
  - `d999cb6 chore: initialize imx6ull sense terminal project`
- 本 session 暂未 push remote/origin。

主要文件：

```text
README.md
HANDOFF.md
.gitignore
app/daemon/main.c
app/daemon/Makefile
config/config.json
systemd/imx6ull-sense.service
scripts/deploy_placeholder.sh
docs/00_current_environment.md
docs/01_bringup.md
docs/02_camera_capture.md
docs/03_mjpeg_stream.md
docs/04_motion_event.md
docs/05_fault_injection.md
docs/test_report.md
docs/interview_qa.md
docs/beginner_guide.md
docs/stage_summaries/README.md
docs/stage_summaries/M0_environment_and_board_baseline.md
docs/bug_reports/README.md
docs/bug_reports/M0-NET-WIFI-001_usb_rndis_board_link.md
docs/plans/plan_v0.1_baseline.md
docs/plans/plan_v0.2_open_source_route.md
docs/plans/plan_v0.3_mvp_execution.md
docs/reference/ustreamer_notes.md
docs/reference/mjpg_streamer_notes.md
docs/reference/motion_notes.md
docs/reference/design_decisions.md
docs/reference/benchmark_against_open_source.md
docs/reference/local_board_documents.md
```

本地忽略产物：

```text
app/daemon/imx6ull-sense
app/daemon/main.o
secrets.local.md
```

## 3. 已确认环境事实

WSL 可用：

```text
Ubuntu on WSL2
Linux ERICHOU 6.6.87.2-microsoft-standard-WSL2
```

WSL 构建工具已安装并验证。ARM scaffold 已完成交叉编译，并已在板端运行成功。

M0 后默认板端连接：

```text
USB RNDIS: debian@192.168.7.2
```

之前的 Wi-Fi 路径 `192.168.18.210` 不稳定，不再作为默认部署路径。

相关记录：

```text
docs/00_current_environment.md
docs/beginner_guide.md
docs/01_bringup.md
docs/stage_summaries/M0_environment_and_board_baseline.md
docs/bug_reports/M0-NET-WIFI-001_usb_rndis_board_link.md
```

## 4. 新 session 首先要做什么

从 WSL 内部进入仓库：

```sh
cd /home/eric/projects/imx6ull-sense-terminal
git status
```

当前应处于 M1 分支：

```text
## codex/m1-uvc-capture
```

使用 USB RNDIS 确认板端可达：

```sh
ping -c 4 192.168.7.2
ssh debian@192.168.7.2 'uname -a; ip addr show usb0'
```

不要在 M1 完成前进入 M2/MJPEG 实现。M1 必须先有真实 UVC 枚举和一帧采集证据。

M1 首批命令：

```sh
ssh debian@192.168.7.2 'ls -l /dev/video*; v4l2-ctl --list-devices'
ssh debian@192.168.7.2 'v4l2-ctl -d /dev/videoX --list-formats-ext'
ssh debian@192.168.7.2 'v4l2-ctl -d /dev/videoX --stream-mmap --stream-count=1 --stream-to=/tmp/m1_first_frame.yuv; ls -l /tmp/m1_first_frame.yuv; wc -c /tmp/m1_first_frame.yuv'
```

把真实输出写入：

```text
docs/02_camera_capture.md
```

## 5. 开源参考策略

项目参考成熟开源项目的架构和工程取舍，但不复制其代码。

参考对象：

- uStreamer / PiKVM：轻量 V4L2 MJPEG HTTP daemon。
- mjpg-streamer：嵌入式 MJPEG streaming 的 input/output 分离。
- Motion：motion event、threshold 和 cooldown 模型。
- v4l-utils：V4L2 bring-up 和验证工具。
- libjpeg-turbo：JPEG 性能参考。

重要规则：

> 开源项目用于架构参考和 benchmark，不直接搬代码；学生 MVP 要自己实现。

避免直接复制 GPL 代码。

## 6. MVP 里程碑

### M0 — 环境与板端基线

交付：

- 构建工具已安装。
- 当前 scaffold 可在 WSL 构建。
- 板端登录和传输路径已确认。
- `docs/01_bringup.md` 已写入真实日志。
- M0 阶段总结和 bug 复盘已补齐。

状态：已完成。

### M1 — USB UVC 摄像头采集

交付：

- 至少一个 `/dev/videoX` 可用。
- USB UVC 是 MVP 默认路线。
- OV5640 作为 UVC 闭环完成后的可选增强项。
- 记录 `v4l2-ctl --list-devices` 和 `--list-formats-ext`。
- 捕获一帧真实图像。
- 更新 `docs/02_camera_capture.md`。
- 完成后写 `docs/stage_summaries/M1_usb_uvc_camera_capture.md`。

状态：进行中。

### M2 — MJPEG 浏览器推流

交付：

- `GET /` HTML 页面。
- `GET /stream` MJPEG stream。
- `GET /status` JSON 状态。
- 浏览器能看到实时画面。
- `docs/03_mjpeg_stream.md` 写入 fps/CPU 记录。

### M3 — Motion Event

交付：

- 简单 Y-frame-diff motion detection。
- JSONL event log。
- `/status` 展示 motion 状态和 event count。
- `docs/04_motion_event.md` 更新。

### M4 — systemd 与故障注入

交付：

- `systemd/imx6ull-sense.service` 能在板端运行。
- `kill -9` 后服务可恢复。
- reboot 后服务自启动。
- 摄像头缺失时进入可解释 degraded 状态。
- 坏配置时 fail-safe。
- `docs/05_fault_injection.md` 和 `docs/test_report.md` 更新。

## 7. 范围控制规则

硬优先级：

1. 真实板端运行。
2. 浏览器可见 stream。
3. motion event log。
4. systemd / 故障注入证据。
5. README / demo / 简历包装。

Fallback 规则：

- USB UVC 是 MVP 默认路线。
- OV5640 可在 UVC 闭环完成后作为 DVP/CSI 增强项尝试。
- OV5640 两个晚上不通就停止该增强项，继续 UVC MVP。
- HDMI 卡住一个晚上就放弃 HDMI。
- libjpeg/JPEG 太慢就降低分辨率、quality 或 fps。
- systemd 临时卡住时，可短期用 shell watchdog 记录现象，但最终必须回到 systemd。
- 如果后期浏览器 stream 仍不工作，停止所有可选工作。

MVP 后才考虑的可选项：

- OV5640 DTS/CSI 深挖。
- HDMI framebuffer 预览。
- `alarm_gpio` 字符设备驱动。
- 应用 OTA。
- TTFF 测量。
- tiny CNN / NCNN / TFLite。

## 8. 文档纪律

项目使用四层文档：

1. 里程碑证据文档：`docs/01_bringup.md`、`docs/02_camera_capture.md` 等。
2. 阶段总结：每个完成里程碑在 `docs/stage_summaries/` 下写一份。
3. bug 复盘：每个 meaningful bug 在 `docs/bug_reports/` 下写一份，并维护跨阶段索引 `docs/bug_reports/README.md`。
4. 本地板级资料索引：`docs/reference/local_board_documents.md`。

规则：

- 每完成一个里程碑，进入下一阶段实现前必须写阶段总结。
- 每处理一个 meaningful bug 或 blocker，必须写 bug 复盘，包含现象、日志、原因、修复、验证和状态。
- 如果某阶段有 bug，阶段总结和 bug 索引都要链接该报告。
- 真实命令输出保留在里程碑证据文档中，阶段总结只提炼结论。
- 阶段证据文档和阶段总结未更新前，不要标记该阶段完成。
- 涉及板级硬件、pinout、启动、CSI、USB、clock、GPIO、电源或设备树的问题，必须先查 `docs/reference/local_board_documents.md`，并记录引用的本地资料。

## 9. 用户工作节奏

用户通常可投入时间：

- 工作日晚上：约 2 小时，有时不可用。
- 周末：时间更多。
- DDL 目标：2026 年 7 月底前完成。

计划要保持弹性。优先按里程碑推进，不要做过死的每日排期。

每次工作 session 结束都要记录：

- 本次尝试了什么。
- 跑了什么命令。
- 看到了什么输出或日志。
- 是否满足验收标准。
- 下一步唯一动作。

## 10. 下一位 agent 的第一动作

1. 读本文件 `HANDOFF.md`。
2. 读：

```text
docs/plans/plan_v0.3_mvp_execution.md
docs/reference/local_board_documents.md
docs/stage_summaries/README.md
docs/bug_reports/README.md
docs/02_camera_capture.md
```

3. 确认当前分支是 `codex/m1-uvc-capture`。
4. 使用 USB RNDIS `192.168.7.2` 验证板端可达。
5. 插入 USB UVC 摄像头后执行 M1 枚举和一帧采集命令。
6. 把真实输出写入 `docs/02_camera_capture.md`。
7. 如果出现 blocker，创建独立 bug 报告并更新 `docs/bug_reports/README.md`。
8. M1 完成后写 `docs/stage_summaries/M1_usb_uvc_camera_capture.md`。

不要在 M1 真实枚举和一帧采集完成前直接写 MJPEG/V4L2 大段实现。

