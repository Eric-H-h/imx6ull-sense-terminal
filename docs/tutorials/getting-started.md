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

先阅读 [架构总览](../architecture/overview.md) 和 [当前计划](../plans/current.md)。综合结论见 [测试报告](../verification/test-report.md)。

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

按照 [连接和部署开发板](../how-to/connect-and-deploy-board.md) 验证 OTG、RNDIS、SSH 和 SCP。不要启用 `autowifi.service`。

## 4. 理解已经完成的阶段

- [M0](../stage_summaries/M0_environment_and_board_baseline.md)：主机、交叉编译、网络、部署和板端运行。
- [M1](../stage_summaries/M1_usb_uvc_camera_capture.md)：UVC 节点识别、格式枚举和首帧采集。
- [M2](../stage_summaries/M2_mjpeg_browser_stream.md)：浏览器 MJPEG 和 `/status`。
- [M3](../stage_summaries/M3_motion_event_logging.md)：motion event 与 JSONL。
- [M4](../stage_summaries/M4_systemd_fault_injection.md)：systemd 与故障注入。
- [M5](../stage_summaries/M5_resume_demo_packaging.md)：测试报告、Demo 和发布包装。

遇到类似问题时，先阅读 [Bug 复盘索引](../bug_reports/README.md)，不要重复从错误假设开始排查。

## 5. 运行正式服务

正式安装和启停见 [服务生命周期](../operations/runbooks/service-lifecycle.md)。临时调试仍可用 [运行和检查 MJPEG 服务](../how-to/run-mjpeg-stream.md)。

浏览器：

```text
http://192.168.7.2:8080/
```

```sh
curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status
```

现场演示按 [Demo 脚本](../presentation/demo-script.md)。

## 6. 保存工作

每次工作结束前：

1. 判断是否满足当前阶段验收标准。
2. 把真实输出放入对应 evidence，而不是复制到多个文档。
3. meaningful bug 写独立复盘并更新索引。
4. 阶段完成后更新阶段总结。
5. 按 [Git 工作流](../how-to/git-workflow.md)检查并暂存明确文件。
