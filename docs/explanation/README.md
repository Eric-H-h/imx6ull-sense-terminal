# 原理说明

本目录回答“为什么”和“如何工作”，不承担逐步操作指导。

- [摄像头采集链路](camera-pipeline.md)：UVC、V4L2、MMAP、MJPEG 与 OV5640 边界。
- [MJPEG over HTTP](mjpeg-over-http.md)：multipart boundary、JPEG part 和浏览器刷新。
- [Motion Detection](motion-detection.md)：3 FPS 灰度帧差、两层阈值和 cooldown。

尚未实现或未验证的内容必须明确标记为 Planned，不能写成当前事实。操作步骤见 [`how-to/`](../how-to/)，设计选择见 [ADR](../architecture/decisions/)。
