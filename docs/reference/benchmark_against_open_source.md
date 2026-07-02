# Benchmark Against Open-Source Streamers

This file is a template for later board-side comparison.

## Goals

- Prove the project is not designed in isolation.
- Compare our implementation with at least one mature streamer.
- Explain why the final resume project still uses our own implementation.

## Candidate Baselines

- uStreamer
- mjpg-streamer

## Metrics

| Item | Our daemon | uStreamer/mjpg-streamer | Notes |
|---|---:|---:|---|
| Resolution | TBD | TBD | |
| FPS | TBD | TBD | |
| CPU usage | TBD | TBD | |
| Memory usage | TBD | TBD | |
| Single-client stability | TBD | TBD | |
| Multi-client behavior | TBD | TBD | |
| Code size / complexity | TBD | TBD | |

## Commands

Record exact commands here after board verification.

```sh
# Our daemon
./imx6ull-sense -c config/config.json

# Reference streamer
# TBD after install/build
```

## Interview Conclusion Template

Mature streamers are stronger production tools, but this project intentionally reimplemented a minimal path to demonstrate V4L2, JPEG encoding, HTTP streaming, motion events and systemd reliability on the target board.

