# 简历要点

三条均可被仓库证据支撑。不要改写成未实现的能力（H.264、AI、OV5640、公网服务）。

## 中文

- 在 i.MX6ULL / EBF6ULL 上自研最小感知 daemon：UVC V4L2 MJPEG 采集、HTTP `/` `/stream` `/status`，单客户端 30 分钟推流保持 30 FPS，RSS 无增长。
- 实现可解释 motion event：3 FPS 灰度帧差、可配置 threshold/cooldown、JSONL 追加；静止 5 分钟 0 误报，挥手 10/10 识别。
- 将服务接入 systemd 无人值守运行，并用故障注入验证 `kill -9` 拉起、非法配置退出码 78 不风暴、摄像头缺失 degraded 自恢复、reboot 后 JSONL 持久化。

## English

- Built a minimal i.MX6ULL daemon for UVC V4L2 MJPEG capture and HTTP `/` `/stream` `/status`; 30-minute single-client run held 30 FPS with no RSS growth.
- Implemented explainable motion events (3 FPS grayscale frame diff, threshold, cooldown, JSONL); 0 false positives in 5 minutes idle, 10/10 wave detections.
- Packaged the daemon as a systemd service and verified crash restart, config fail-safe (exit 78), camera-missing degraded mode, and JSONL persistence across reboot.
