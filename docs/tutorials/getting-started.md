# 从零理解并运行项目

本教程面向第一次进入仓库的学习者。它提供阅读和执行顺序，不复制每个操作指南的完整命令。

## 1. 理解项目目标

项目要在 EBF6ULL S1 Pro / i.MX6ULL 上完成以下闭环：

```text
USB UVC 摄像头
  -> V4L2 采集
  -> MJPEG 浏览器预览
  -> motion event
  -> JSONL 日志
  -> systemd 与故障注入
```

先阅读 [架构总览](../architecture/overview.md) 和 [当前计划](../plans/current.md)。

## 2. 准备开发环境

仓库路径：

```sh
cd /home/eric/projects/imx6ull-sense-terminal
```

按照 [准备 WSL 开发环境](../how-to/setup-development-host.md) 验证主机编译和 ARM 交叉编译。

## 3. 连接开发板

默认使用 USB RNDIS：

```text
debian@192.168.7.2
```

按照 [连接和部署开发板](../how-to/connect-and-deploy-board.md) 验证 OTG、RNDIS、SSH 和 SCP。

## 4. 理解已经完成的阶段

- [M0 阶段总结](../stage_summaries/M0_environment_and_board_baseline.md)：主机、交叉编译、网络、部署和板端运行。
- [M1 阶段总结](../stage_summaries/M1_usb_uvc_camera_capture.md)：UVC 节点识别、格式枚举和首帧采集。

遇到类似问题时，先阅读 [Bug 复盘索引](../bug_reports/README.md)，不要重复从错误假设开始排查。

## 5. 继续当前 M2

当前代码已经包含 V4L2 MJPEG 采集、共享最新帧、HTTP `/`、`/stream` 和 `/status`，但 M2 仍需要代码修复、板端和浏览器验收。

执行入口见 [运行和检查 MJPEG 服务](../how-to/run-mjpeg-stream.md)，验收结果写入 [M2 evidence](../verification/evidence/M2_mjpeg_stream.md)。

## 6. 保存工作

每次工作结束前：

1. 判断是否满足当前阶段验收标准。
2. 把真实输出放入对应 evidence，而不是复制到多个文档。
3. meaningful bug 写独立复盘并更新索引。
4. 阶段完成后更新阶段总结。
5. 按 [Git 工作流](../how-to/git-workflow.md)检查并暂存明确文件。
