# HANDOFF — i.MX6ULL Sense Terminal

This handoff is for a new main Codex/Claude session that will continue the project under:

```text
WSL path:     /home/eric/projects/imx6ull-sense-terminal
Windows path: \\wsl.localhost\Ubuntu\home\eric\projects\imx6ull-sense-terminal
```

## 1. Current Objective

Build a small but complete embedded Linux resume project on the EmbedFire EBF6ULL S1 Pro / i.MX6ULL eMMC board.

The project must be practical and closed-loop, because the user's previous peRTOS project was mostly source-code analysis and lacked real board-side implementation.

MVP target:

```text
V4L2 camera/UVC capture
  -> JPEG/MJPEG browser stream
  -> simple motion event detection
  -> local event log
  -> systemd restart/watchdog behavior
  -> fault-injection documentation
```

Do not expand the project into a large video-surveillance platform. The value is a board-verified, explainable, reliable embedded Linux service.

## 2. Current Repository State

Repository has been created and scaffolded at the WSL path above.

Git:

- `git init` has been run.
- Branch was renamed to `main`.
- No initial commit has been made.
- Do not set Git username/email for the user unless explicitly asked.

Generated files include:

```text
README.md
HANDOFF.md
.gitignore
app/daemon/main.c
app/daemon/Makefile
config/config.json
systemd/imx6ull-sense.service
scripts/deploy_placeholder.sh
docs/00_current_environment.md
docs/01_bringup.md
docs/02_camera_capture.md
docs/03_mjpeg_stream.md
docs/04_motion_event.md
docs/05_fault_injection.md
docs/test_report.md
docs/interview_qa.md
docs/beginner_guide.md
docs/plans/plan_v0.1_baseline.md
docs/plans/plan_v0.2_open_source_route.md
docs/reference/ustreamer_notes.md
docs/reference/mjpg_streamer_notes.md
docs/reference/motion_notes.md
docs/reference/design_decisions.md
docs/reference/benchmark_against_open_source.md
```

## 3. Environment Facts Already Discovered

WSL is available:

```text
Ubuntu on WSL2
Linux ERICHOU 6.6.87.2-microsoft-standard-WSL2
```

But basic build tools are missing:

```text
make: not installed
gcc: not installed
```

This is recorded in:

```text
docs/00_current_environment.md
docs/beginner_guide.md
```

## 4. First Commands For The New Session

Start from:

```sh
cd /home/eric/projects/imx6ull-sense-terminal
git status
```

Install base build tools:

```sh
sudo apt update
sudo apt install -y build-essential git
```

Verify:

```sh
make --version
gcc --version
git --version
```

Build and run scaffold:

```sh
make -C app/daemon
./app/daemon/imx6ull-sense -c config/config.json
```

Then let the user configure Git identity and make the first commit:

```sh
git config user.name "侯子豪"
git config user.email "USER_EMAIL_HERE"
git add .
git commit -m "chore: initialize imx6ull sense terminal project"
```

Do not guess the email address.

## 5. Open-Source Reference Strategy

The project references mature open-source projects for architecture and engineering choices, but does not copy their code.

References:

- uStreamer / PiKVM: lightweight V4L2 MJPEG HTTP daemon.
- mjpg-streamer: input/output separation for embedded MJPEG streaming.
- Motion: motion-event model, threshold and cooldown behavior.
- v4l-utils: V4L2 bring-up and verification.
- libjpeg-turbo: JPEG performance reference.

Important rule:

> Use open-source projects as architecture references and benchmark baselines. Implement the student MVP yourself.

Avoid direct GPL code copying.

## 6. MVP Milestones

### M0 — Environment and Board Baseline

Deliver:

- Build tools installed.
- `hello` or current scaffold builds in WSL.
- Board login and transfer path confirmed.
- `docs/01_bringup.md` updated with real logs.

### M1 — Camera Capture

Deliver:

- At least one `/dev/videoX` works.
- Prefer OV5640, but if it blocks for two evenings, switch to USB UVC.
- `v4l2-ctl --list-devices` and `--list-formats-ext` recorded.
- One real frame captured.
- `docs/02_camera_capture.md` updated.

### M2 — MJPEG Browser Stream

Deliver:

- `GET /` HTML page.
- `GET /stream` MJPEG stream.
- `GET /status` JSON status.
- Browser can view live frames.
- `docs/03_mjpeg_stream.md` updated with fps/CPU notes.

### M3 — Motion Event

Deliver:

- Simple Y-frame-diff motion detection.
- JSONL event log.
- `/status` shows motion state and event count.
- `docs/04_motion_event.md` updated.

### M4 — systemd and Fault Injection

Deliver:

- `systemd/imx6ull-sense.service` works on board.
- Service restarts after `kill -9`.
- Reboot autostart verified.
- Camera missing -> degraded behavior.
- Bad config -> fail-safe behavior.
- `docs/05_fault_injection.md` and `docs/test_report.md` updated.

## 7. Scope Control Rules

Hard priorities:

1. Real board execution.
2. Browser-visible stream.
3. Motion event log.
4. systemd/fault-injection evidence.
5. README/demo/resume packaging.

Fallback rules:

- OV5640 blocks for two evenings -> switch to USB UVC.
- HDMI blocks for one evening -> drop HDMI from MVP.
- libjpeg/JPEG is slow -> reduce resolution/quality/fps.
- systemd blocks -> temporarily use a shell watchdog, then return to systemd.
- If browser stream is not working by the late stage, stop all optional work.

Optional only after MVP:

- OV5640 DTS/CSI deep dive.
- HDMI framebuffer preview.
- `alarm_gpio` character driver.
- Application OTA.
- TTFF measurement.
- tiny CNN / NCNN / TFLite.

## 8. User Working Pattern

The user can usually work:

- Weekday evenings: about 2 hours, sometimes unavailable.
- Weekends: more time available.
- DDL target: finish by the end of July 2026.

Plans should be flexible. Prefer milestone-based progress over rigid daily schedules.

Every work session should end with:

- What was attempted.
- What command was run.
- What output/log was observed.
- Whether the success criterion was met.
- Next single step.

## 9. What The Next Agent Should Do First

1. Read this `HANDOFF.md`.
2. Read:

```text
docs/plans/plan_v0.2_open_source_route.md
docs/reference/design_decisions.md
docs/beginner_guide.md
docs/00_current_environment.md
```

3. Install build tools in WSL if user approves/runs it.
4. Build `app/daemon` scaffold.
5. Guide the user through the first commit.
6. Then start M0 board bring-up documentation.

Do not jump directly into V4L2 code until the build/deploy loop is confirmed.

