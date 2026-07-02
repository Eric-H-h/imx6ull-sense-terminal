# Interview Q&A

## Q1: Why not directly use uStreamer or mjpg-streamer?

Because the goal is not to ship a production streamer, but to build and verify the core embedded Linux path myself: V4L2 capture, JPEG encoding, HTTP streaming, event detection and systemd reliability. I used mature projects as architecture references and kept my implementation small enough to explain and debug.

## Q2: Why MJPEG instead of H.264/RTSP?

i.MX6ULL has no hardware H.264 encoder. Software H.264 is too heavy for the main path. MJPEG-over-HTTP is simple, browser-friendly and good enough for a LAN demo. The tradeoff is higher bandwidth.

## Q3: What is the most valuable engineering point?

The project is not only a video demo. It is a board-verified device service with status, event logs, systemd restart and fault-injection evidence.

## Q4: What did you borrow from open-source projects?

uStreamer inspired the light MJPEG daemon shape, mjpg-streamer inspired input/output separation, Motion inspired event state and cooldown, and v4l-utils provided the camera verification path.

## Q5: What did you implement yourself?

The minimal capture/encode/HTTP/event/log/service path, plus board-side validation and failure-mode documentation.

