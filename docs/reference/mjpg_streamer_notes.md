# mjpg-streamer Notes

Reference: <https://github.com/jacksonliam/mjpg-streamer>

## What It Is

mjpg-streamer is a classic embedded MJPEG streaming project. It is useful as a reference because it targets resource-constrained Linux devices and separates frame input from output.

## Ideas To Borrow

- Decouple input and output.
- Keep the capture side independent from the transport side.
- Make MJPEG-over-HTTP the simple browser-facing demo path.
- Prefer practical low-resource choices over heavy media frameworks in the final project.

## Ideas Not To Borrow For MVP

- A full plugin framework.
- Multiple input/output plugins.
- Full compatibility options and old device paths.

## Project Mapping

Our implementation keeps a simpler variant:

```text
capture_v4l2 -> frame_buffer -> http_server
```

This gives the same engineering separation without turning the student project into a framework.

