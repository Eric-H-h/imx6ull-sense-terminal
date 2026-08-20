# HANDOFF：i.MX6ULL Sense Terminal

本文件给继续主线执行的 session 提供当前事实和唯一下一步。详细知识通过链接进入对应事实源，不在本文件复制完整日志。

## 项目路径

```text
WSL: /home/eric/projects/imx6ull-sense-terminal
Windows: \\wsl.localhost\Ubuntu\home\eric\projects\imx6ull-sense-terminal
```

Git 命令必须在 WSL 路径执行。

## 当前状态

- 当前分支：`codex/m4-systemd-fault`。
- M0、M1、M2、M3：Completed，并已合入 `develop`。
- M4：正式安装、生命周期、`kill -9`、非法配置、事件日志写失败和 reboot 自启均已验收；等待审查、提交与 PR。
- M5：未开始。
- 默认摄像头路线：USB UVC。
- OV5640：MVP 后可选增强项。
- 默认板端连接：`debian@192.168.7.2`，USB RNDIS。

M4 工作区包含尚未提交的源码、脚本、unit、测试和文档；`.vscode/`、`tmp/` 和 `.grok/` 是无关或本机权限文件。不要删除来源不明的文件，不要执行 `git add .`。

## 当前实现边界

M3 能力仍在，M4 已补上无人值守运行：

- 动态选择 UVC Capture 节点并以 MJPEG 640x480@30 推流。
- motion：3 FPS 抽样、灰度帧差、threshold、cooldown、JSONL。
- 正式路径：`/usr/local/bin/imx6ull-sense`、`/etc/imx6ull-sense/config.json`、`/var/lib/imx6ull-sense/`、`imx6ull-sense.service`。
- `Restart=on-failure`；配置错误退出码 78 不自动重启。
- 摄像头缺失或事件日志写失败时服务保持运行并报告 degraded。

未实现或未完成：

- M5 测试报告、Demo 和发布包装。
- OV5640 CSI/DVP。

## 下一步唯一主线

收尾 M4，不提前进入 M5：

1. 审查 M4 源码、测试、脚本、unit、文档和未跟踪文件清单。
2. 按明确文件清单暂存并提交，不使用 `git add .`。
3. 推送 `codex/m4-systemd-fault` 并创建面向 `develop` 的 PR。
4. PR 合并后，本地 fast-forward 到最新 `develop`。
5. 只有工作区干净后才创建 `codex/m5-docs-demo`。

M4 结果见 [M4 evidence](docs/verification/evidence/M4_fault_injection.md) 与 [M4 阶段总结](docs/stage_summaries/M4_systemd_fault_injection.md)。操作见 [服务生命周期](docs/operations/runbooks/service-lifecycle.md)。

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
- 不要启用 `autowifi.service`。Wi-Fi/SDIO 阻塞见 [M4-DRV-WIFI-001](docs/bug_reports/M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md)。

## 文档纪律

- 实际命令和输出：`docs/verification/evidence/`。
- 阶段完成结论：`docs/stage_summaries/`。
- 学习型调试复盘：`docs/bug_reports/`。
- 正式运行事故：`docs/operations/postmortems/`。
- 当前执行顺序：`docs/plans/current.md`。
- 路线和长期选择原因：`docs/architecture/decisions/`。
- 正式服务操作：`docs/operations/runbooks/`。

不要再引用迁移前的根级编号文档、版本化当前计划或集中式设计决策汇总；所有入口以 `docs/README.md` 为准。
