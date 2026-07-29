# HANDOFF：i.MX6ULL Sense Terminal

本文件给继续主线执行的 session 提供当前事实和唯一下一步。详细知识通过链接进入对应事实源，不在本文件复制完整日志。

## 项目路径

```text
WSL: /home/eric/projects/imx6ull-sense-terminal
Windows: \\wsl.localhost\Ubuntu\home\eric\projects\imx6ull-sense-terminal
```

Git 命令必须在 WSL 路径执行。

## 当前状态

- 当前分支：`codex/m3-motion-event`。
- M0、M1、M2：Completed，并已合入 `develop`。
- M3：功能、代码检查和板端验收 Completed，等待 commit 与 PR。
- M4-M5：未开始。
- 默认摄像头路线：USB UVC。
- OV5640：MVP 后可选增强项。
- 默认板端连接：`debian@192.168.7.2`，USB RNDIS。

M3 工作区包含尚未提交的源码、测试、构建入口和文档；`.vscode/` 与 `tmp/` 是无关未跟踪内容。不要删除来源不明的文件，不要执行 `git add .`。

## 当前实现边界

M3 已实现并通过验收：

- 动态选择 UVC Capture 节点并以 MJPEG 640x480@30 推流。
- latest JPEG 以 3 FPS 抽样，libjpeg 缩放解码到 grayscale。
- 帧差 score、两层 threshold、cooldown 和 JSONL event log。
- `/status` 输出真实 motion state、score、采样 FPS 和 event count。
- camera missing/recovery 时清除 baseline，恢复第一帧不产生假事件。
- 一个浏览器客户端与 motion 并行运行30分钟，stream 30 FPS、RSS delta 0。

未实现或未完成：

- M4 systemd 正式部署和故障注入。
- M5 发布包装。
- OV5640 CSI/DVP。

## 下一步唯一主线

收尾 M3，不提前进入 M4：

1. 审查 M3 源码、测试、文档和未跟踪文件清单。
2. 按明确文件清单暂存并提交，不使用 `git add .`。
3. 推送 `codex/m3-motion-event` 并创建面向 `develop` 的 PR。
4. PR 合并后，本地 fast-forward 到最新 `develop`。
5. 只有工作区干净后才创建 `codex/m4-systemd-fault`。

M3 结果见 [M3 evidence](docs/verification/evidence/M3_motion_event.md) 与 [M3 阶段总结](docs/stage_summaries/M3_motion_event_logging.md)。

## 必读入口

1. [当前计划](docs/plans/current.md)
2. [架构总览](docs/architecture/overview.md)
3. [组件说明](docs/architecture/components.md)
4. [Git 工作流](docs/how-to/git-workflow.md)
5. [本地板级资料](docs/reference/hardware/local-board-documents.md)
6. [Bug 索引](docs/bug_reports/README.md)

## 硬件和 Bug 约束

- 涉及 EBF6ULL 接口、pinout、USB、CSI、时钟、电源或设备树时，先查本地资料，原理图作为板级事实源。
- 不把 `/dev/videoX` 编号当作稳定身份；按 driver 和 Device Caps 选择 UVC 图像节点。
- 当前 BSP 查询 PXP Video Output 节点曾触发内核 Oops，不要把 PXP 节点当作摄像头。
- 网络失败按 OTG 接口、数据线、Windows RNDIS、`usb0` carrier、IP/SSH 的顺序排查。

## 文档纪律

- 实际命令和输出：`docs/verification/evidence/`。
- 阶段完成结论：`docs/stage_summaries/`。
- 学习型调试复盘：`docs/bug_reports/`。
- 正式运行事故：`docs/operations/postmortems/`。
- 当前执行顺序：`docs/plans/current.md`。
- 路线和长期选择原因：`docs/architecture/decisions/`。

不要再引用迁移前的根级编号文档、版本化当前计划或集中式设计决策汇总；所有入口以 `docs/README.md` 为准。
