# 里程碑验收证据

| 阶段 | 状态 | 证据 |
| --- | --- | --- |
| M0 | Completed | [环境、交叉编译和板端 bring-up](M0_bringup.md) |
| M1 | Completed | [USB UVC 枚举和首帧](M1_uvc_capture.md) |
| M2 | Completed | [MJPEG 浏览器推流](M2_mjpeg_stream.md) |
| M3 | Completed | [Motion Event](M3_motion_event.md) |
| M4 | Completed | [systemd 与故障注入](M4_fault_injection.md) |
| M5 | Completed | 无新的板端测试；综合结论见 [test-report.md](../test-report.md) |

## 记录规则

- 记录真实执行命令、关键输出、测试环境和判断结果。
- 不补写无法从会话或日志确认的数值。
- 大体积抓帧、完整 `dmesg` 和可重建产物不直接提交；文档保存支撑结论的摘要和哈希或附件位置。
- meaningful bug 链接对应 bug report，不在 evidence 中重复完整定位过程。
