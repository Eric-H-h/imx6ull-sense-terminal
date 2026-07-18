# HANDOFF：i.MX6ULL Sense Terminal

本文件给继续主线执行的 session 提供当前事实和唯一下一步。详细知识通过链接进入对应事实源，不在本文件复制完整日志。

## 项目路径

```text
WSL: /home/eric/projects/imx6ull-sense-terminal
Windows: \\wsl.localhost\Ubuntu\home\eric\projects\imx6ull-sense-terminal
```

Git 命令必须在 WSL 路径执行。

## 当前状态

- 当前分支：`codex/m2-mjpeg-stream`。
- M0：Completed。
- M1：Completed，并已合入 `develop`。
- M2：In Progress，代码实现后审查和板端验收前。
- M3-M5：未开始。
- 默认摄像头路线：USB UVC。
- OV5640：MVP 后可选增强项。
- 默认板端连接：`debian@192.168.7.2`，USB RNDIS。

当前工作区包含尚未提交的 M2 源码和文档重构改动。不要删除来源不明的文件，不要切分支，不要执行 `git add .`。

## 当前实现边界

已实现但仍待完整验收：

- 动态选择 `uvcvideo + Video Capture + Streaming` 节点。
- V4L2 MJPEG MMAP 采集。
- AppState 最新 JPEG 和运行状态。
- `GET /`、`GET /stream`、`GET /status`。
- 摄像头失败 degraded 和重新扫描。

未实现或未完成：

- M2 完整代码修复和板端验收。
- M3 motion detection 和 JSONL event log。
- M4 systemd 正式部署和故障注入。
- OV5640 CSI/DVP。

`/status` 中的 motion 字段在 M3 前是占位值，不代表 motion 已实现。

## 下一步唯一主线

完成 M2，不进入 M3：

1. 结束当前 M2 代码审查并修复阻塞问题。
2. 主机干净构建和 ARM 交叉编译。
3. 通过 USB RNDIS 部署到板端。
4. 验证 `/`、`/status`、`/stream` 协议和浏览器动态画面。
5. 验证刷新、断开、重连、camera missing 和恢复。
6. 运行 30 分钟并记录 FPS、CPU、RSS 和内核日志。
7. 更新 M2 evidence、Bug 索引和 M2 阶段总结。
8. 核对提交范围后再 commit、push 和创建 M2 MR/PR。

命令和验收表见 [M2 操作指南](docs/how-to/run-mjpeg-stream.md) 与 [M2 evidence](docs/verification/evidence/M2_mjpeg_stream.md)。

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
