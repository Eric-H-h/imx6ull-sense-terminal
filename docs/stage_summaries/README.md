# 阶段总结索引

本目录用于保存每个已完成里程碑的阶段总结文档。

## 记录规则

每完成一个里程碑，都必须先在本目录补齐阶段总结，再进入下一个里程碑的实现工作。

每份阶段总结至少包含：

- 里程碑名称和完成日期
- 阶段范围和验收结果
- 验收证据和真实命令输出
- 本阶段遇到的 bug 或 blocker
- 本阶段发生的实际运行事故
- 未关闭风险和缓解方式
- 本阶段做出的关键决策
- 已更新的文档
- Git 分支和 commit
- 进入下一阶段的条件

其中：

- 搭建、开发和调试中的学习型问题链接到 `docs/bug_reports/`。
- 正式或长期运行中产生实际影响的事故链接到 `docs/operations/postmortems/`。
- 没有运行事故时明确写“无”，不能把普通开发 Bug 填入事故栏。

新总结使用 [`_template.md`](_template.md)。

## 命名规则

使用稳定的里程碑文件名：

```text
M0_environment_and_board_baseline.md
M1_usb_uvc_camera_capture.md
M2_mjpeg_browser_stream.md
M3_motion_event_logging.md
M4_systemd_fault_injection.md
M5_resume_demo_packaging.md
```

## 当前阶段总结

| 里程碑 | 总结文档 | 状态 |
| --- | --- | --- |
| M0 | [M0 环境与板端基线](M0_environment_and_board_baseline.md) | 已完成 |
| M1 | [M1 USB UVC 摄像头采集](M1_usb_uvc_camera_capture.md) | 已完成 |
| M2 | [M2 MJPEG 浏览器推流](M2_mjpeg_browser_stream.md) | 已完成 |
| M3 | [M3 Motion Event 与 JSONL](M3_motion_event_logging.md) | 已完成 |
| M4 | [M4 systemd 与故障注入](M4_systemd_fault_injection.md) | 已完成 |
| M5 | [M5 测试报告、演示和发布包装](M5_resume_demo_packaging.md) | 已完成，待提交 |
