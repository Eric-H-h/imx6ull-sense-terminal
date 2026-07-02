# Design Decisions

## Decision 1: Build Our Own MVP Instead Of Shipping uStreamer

We use uStreamer as architecture reference, but the final demo is our own implementation.

Reason:

- The resume needs defendable hands-on output.
- A small self-built pipeline is easier to explain in interviews.
- The board has limited resources; we only need one controlled path.

## Decision 2: Use MJPEG-over-HTTP First

Reason:

- Browser-friendly.
- No player installation needed.
- i.MX6ULL has no hardware H.264 encoder.
- Software H.264 is too heavy for the main line.

Tradeoff:

- MJPEG uses more bandwidth than H.264.
- This is acceptable for LAN demo and resume validation.

## Decision 3: Keep Input/Output Decoupled But Avoid Plugins

Borrowed from mjpg-streamer:

```text
input -> shared frame -> output
```

MVP implementation:

```text
capture_v4l2.c -> frame_buffer.c -> http_server.c
```

No plugin ABI in v1.

## Decision 4: Treat Camera Failure As Degraded Mode

The daemon should not crash in the basic failure path.

Expected behavior:

- `/status` shows `"degraded": true`.
- Logs include the failed camera open/init reason.
- systemd may restart only for unexpected fatal errors.

## Decision 5: Motion Detection Is Event-Oriented

Borrowed from Motion:

- Use threshold.
- Use cooldown.
- Write event logs.

MVP avoids recording and complex region masks.

## Decision 6: Keep Open-Source Reference Visible

The repository should document what was referenced and what was intentionally not copied.

This helps interview explanation:

> I read mature projects, extracted the design choices suitable for i.MX6ULL, and implemented a smaller board-verified version myself.

