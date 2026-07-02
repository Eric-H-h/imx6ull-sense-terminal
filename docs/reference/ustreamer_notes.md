# uStreamer Notes

Reference: <https://github.com/pikvm/ustreamer>

## What It Is

uStreamer is a lightweight V4L2 streaming server used by PiKVM. It focuses on serving camera frames over HTTP with low overhead and practical daemon behavior.

## Ideas To Borrow

- Keep the streaming service small and focused.
- Split camera capture, JPEG encoding, HTTP serving and status reporting.
- Provide a simple browser-friendly stream endpoint.
- Expose runtime status for debugging.
- Treat camera failure as a degraded runtime state instead of an immediate hard crash.
- Integrate cleanly with systemd for long-running device deployment.

## Ideas Not To Borrow For MVP

- Full feature set.
- Broad hardware compatibility matrix.
- Full production-grade configuration surface.
- Complex performance tuning before the basic project runs on i.MX6ULL.

## Project Mapping

Our MVP keeps the shape:

```text
capture -> encode -> latest-frame buffer -> HTTP stream/status -> systemd
```

but implements only the smallest path needed for the board and resume project.

