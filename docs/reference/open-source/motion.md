# Motion Project Notes

Reference: <https://github.com/Motion-Project/motion>

## What It Is

Motion is a mature Linux camera motion detection daemon. It is a useful reference for event-oriented behavior rather than for code reuse.

## Ideas To Borrow

- Motion is not just a video viewer; it is an event-driven daemon.
- Motion detection should have states, not just a raw per-frame boolean.
- Use a threshold and cooldown period to reduce repeated event spam.
- Log event start/end or at least event timestamp and score.
- Runtime status and logs matter for debugging.

## Ideas Not To Borrow For MVP

- Recording.
- Complex region masks.
- Web UI.
- Heavy configuration surface.
- Multiple camera management.

## Project Mapping

Our motion logic:

```text
idle -> candidate motion -> motion active -> cooldown -> idle
```

The first implementation can be simpler:

```text
previous Y frame + current Y frame -> diff score -> threshold -> event JSONL
```
