# 操作指南

本目录回答“怎样完成一个明确任务”。每份指南包含前置条件、命令、预期结果和失败后的排查入口；设计原因不在这里重复。

## 当前可执行指南

- [准备 WSL 开发环境](setup-development-host.md)
- [连接和部署开发板](connect-and-deploy-board.md)
- [识别 UVC 节点并采集一帧](capture-uvc-frame.md)
- [运行和检查 MJPEG 服务](run-mjpeg-stream.md)，M2 验收中
- [Git 工作流](git-workflow.md)

## 后续阶段

- Motion 调参与事件验证在 M3 实现后补充。
- systemd 安装和故障注入在 M4 验证后补充到 `operations/runbooks/`。

真实测试输出写入 [`verification/evidence/`](../verification/evidence/)，不要复制到操作指南。
