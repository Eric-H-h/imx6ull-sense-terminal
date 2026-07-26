# M3 验收证据：Motion Event

## 状态

- 状态：Completed
- 分支：`codex/m3-motion-event`
- M2 PR #2、PR #3：已合并
- 当前阶段：motion event、JSONL、状态接口、恢复和稳定性验收完成
- 实现状态：主机与 ARM 构建、单元测试、板端功能和 30 分钟并行运行全部通过

## 输入方案

保留 M2 的 UVC MJPEG pass-through 主链路，从 latest JPEG 低频抽样解码 grayscale。板端 benchmark 最终确认采用 160x120、3 FPS 作为 M3 默认参数；5 FPS 只保留为可调上限。

## Preflight 证据

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| `develop == origin/develop` | Passed | `cbb4630` |
| PR #2 / #3 已合并 | Passed | 两个提交均为 `origin/develop` 祖先 |
| tracked worktree | Passed | tracked 文件干净；本地 `.vscode/`、`tmp/` 不提交 |
| ARM 工具链 | Passed | Linaro GCC 7.5.0，target `arm-linux-gnueabihf` |
| Host libjpeg | Passed | Ubuntu `libjpeg-dev`；探针 x86-64，`version=80`，exit 0 |
| ARM libjpeg 开发文件 | Passed | Debian 10 armhf `1:1.5.2-2+deb10u1` 独立 sysroot |
| ARM ELF | Passed | ELF 32-bit ARM EABI5 |
| ARM 动态依赖 | Passed | `NEEDED libjpeg.so.62` |
| 板端运行 | Passed | `libjpeg_probe_ok version=62`，exit 0 |
| 板端实际库 | Passed | `/usr/lib/arm-linux-gnueabihf/libjpeg.so.62` |

独立 ARM sysroot：

```text
/home/eric/.local/sysroots/imx6ull-libjpeg-deb10-1.5.2-armhf
```

该目录只用于构建，不提交到仓库，也不修改 Linaro 自带 sysroot。

## Preflight Bug / Blocker

- [M3-ENV-PROXY-001](../../bug_reports/M3-ENV-PROXY-001_sudo_apt_drops_proxy.md)：`sudo` 未保留 WSL 代理环境，APT 直连 Ubuntu archive 超时；通过单次 APT proxy 参数解决。
- [M3-OPS-SSH-001](../../bug_reports/M3-OPS-SSH-001_ssh_consumes_piped_script_stdin.md)：通过管道执行的 WSL 自动化脚本中，`ssh` 读取 stdin 并消费后续脚本；改用 `ssh -n`。
- 开发板未通电曾导致 SSH banner timeout；通电后 RNDIS ping、SSH banner 和密钥认证恢复，属于现场状态，不单独建立 Bug 报告。

## JPEG Decoder 与 Benchmark

- 解码接口：内存 JPEG -> 8-bit grayscale。
- libjpeg 输出模式：`JCS_GRAYSCALE`。
- 缩放方式：libjpeg 原生 `1/4` DCT scaling。
- 目标尺寸：640x480 -> 160x120。
- 坏 JPEG：返回 `JPEG_GRAY_INVALID_IMAGE`，不终止进程。
- 输出缓冲区：容量足够时复用，避免每帧重复分配。

主机验证：

```text
jpeg decoder tests: PASS
ASan/UBSan: PASS
GCC -fanalyzer: PASS
gray_size: 160x120
decode_failed: 0
```

ARM benchmark：

```text
ELF 32-bit LSB executable, ARM, EABI5
interpreter: /lib/ld-linux-armhf.so.3
NEEDED: libjpeg.so.62
NEEDED: libc.so.6
```

## 真实 UVC 输入门禁

动态选择条件为 sysfs driver `uvcvideo`、`Device Caps: Video Capture`、支持 `MJPG`。本次枚举结果：

| 节点 | 类型 | 选择 |
| --- | --- | --- |
| `/dev/video1` | UVC Video Capture，支持 MJPG 640x480@30 | 是 |
| `/dev/video2` | UVC Metadata Capture | 否 |

单帧证据：

```text
capture_exit:0
frame_size:12126 bytes
jpeg_soi: ff d8
jpeg_eoi: ff d9
```

板端 3 秒烟雾测试：

```text
gray_size: 160x120
target_fps: 3
decode_success: 9
decode_failed: 0
average_decode_ms: 16.807
max_decode_ms: 19.863
actual_sample_fps: 3.000
benchmark_exit:0
```

## 板端性能矩阵

输入为同一张真实 UVC MJPEG 帧，输出 160x120 grayscale，每档运行 30 秒：

| 目标 FPS | 成功 / 失败 | 平均解码 | 最大解码 | CPU | RSS | 实际 FPS |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 60 / 0 | 19.057 ms | 28.397 ms | 3.6%-3.8% | 412 KB | 2.000 |
| 3 | 90 / 0 | 18.554 ms | 20.910 ms | 5.1%-5.4% | 392 KB | 3.000 |
| 5 | 150 / 0 | 16.606 ms | 20.130 ms | 8.0%-8.8% | 392 KB | 5.000 |

三档均为单线程，`VmSize=1632 KB`，5、15、25 秒采样期间 RSS 未增长。

## M2 并行验证

测试负载：

```text
M2 UVC MJPEG 640x480@30
+ 1 个 MJPEG stream 客户端
+ JPEG grayscale benchmark 160x120@3 FPS，持续 30 秒
```

M2 状态：

| 检查点 | FPS | client_count | degraded | last_error |
| --- | ---: | ---: | --- | --- |
| 客户端连接前 | 29.9 | 0 | false | null |
| 客户端连接后 | 29.9 | 1 | false | null |
| benchmark 结束后 | 30.0 | 0 | false | null |

并行 benchmark：

```text
decode_success: 90
decode_failed: 0
average_decode_ms: 11.775
max_decode_ms: 20.583
actual_sample_fps: 3.000
benchmark_exit:0
```

资源采样：

| 时间 | M2 daemon CPU / RSS | benchmark CPU / RSS |
| ---: | ---: | ---: |
| 5 秒 | 2.5% / 2912 KB | 3.4% / 408 KB |
| 15 秒 | 3.8% / 2912 KB | 3.4% / 408 KB |
| 25 秒 | 4.8% / 2912 KB | 3.5% / 408 KB |

内核日志：

```text
dmesg_before_lines:716
dmesg_after_lines:716
no_new_dmesg_lines
```

证据限制：MJPEG 客户端连接由 `client_count=1` 和稳定 FPS 证明，但 curl 的最终下载字节统计因 [M3-OPS-SSH-001](../../bug_reports/M3-OPS-SSH-001_ssh_consumes_piped_script_stdin.md) 未写入结果文件。M2 阶段已独立完成 multipart/JPEG 字节结构验证，本节不补造缺失数值。

## 参数决定

- 默认 motion sample rate：`3 FPS`。
- 默认 grayscale size：`160x120`。
- `5 FPS`：保留为可调上限，不作为默认值。
- 依据：3 FPS 独立 benchmark CPU 约 5%，与 M2 并行时推流保持 29.9-30.0 FPS，未出现解码失败、RSS 增长或新增内核日志。

## 集成实现

M3 在 M2 latest JPEG 共享边界后增加独立低频 worker，JPEG 数据复制完成后在 AppState 锁外处理：

```text
state_wait_jpeg_snapshot
  -> jpeg_gray_decode
  -> motion_detector_process
  -> motion_event_gate_update
  -> event_log_append_motion
  -> state_update_motion
```

实现模块：

- `jpeg_decoder`：内存 JPEG 到 grayscale，支持 1/2/4/8 scaling 和输出缓冲复用。
- `motion_detector`：逐像素亮度差、changed pixels 和归一化 score。
- `motion_event_gate`：IDLE/COOLDOWN 状态机和本次运行事件计数。
- `event_log`：单事件单行 JSONL，处理短写、EINTR 和写入失败。
- `motion_pipeline`：组合解码、检测、gate 和日志。
- `motion_worker`：按配置抽样、恢复 baseline、发布真实状态。
- `status_json`：独立格式化 `/status` JSON。

## 最终构建与代码检查

Makefile 新增聚合入口：

```sh
make -C app/daemon -n verify
make -C app/daemon verify
```

`verify` 依次执行 clean、主机 daemon 构建和 8 组单元测试，全部返回 PASS。GCC `-fanalyzer` 对 daemon 全部源文件完成检查，无告警。

ARM 构建固化为：

```sh
./scripts/build-arm.sh
```

输出：

```text
ELF 32-bit LSB executable, ARM, EABI5
interpreter /lib/ld-linux-armhf.so.3
NEEDED libjpeg.so.62
NEEDED libpthread.so.0
NEEDED libc.so.6
```

脚本缺少工具链时明确返回 `exit 2`；故意让 `readelf` 中途失败时返回非零，退出 trap 仍清理全部 ARM `.o/.d` 和根目录二进制。ARM 产物固定在 `app/daemon/build/arm/imx6ull-sense`。

## 运动校准

| 场景 | 结果 | 结论 |
| --- | ---: | --- |
| 静止 5 分钟 | `static_delta=0` | 无持续误报 |
| 10 次独立挥手 | `wave_detected=10` | 10/10 命中 |
| 持续运动 | `continuous_events=7` | 1500 ms cooldown 有效限制事件频率 |
| 浏览器并行 | client 1，画面持续更新 | 推流与 motion 可并行 |

校准参数保持为 3 FPS、1/4 scaling、像素差 25、变化比例 5%、cooldown 1500 ms。

## JSONL 与状态接口

板端事件文件为 `/tmp/imx6ull-sense-m3/events.jsonl`。一次恢复测试中，`/status event_count=13`，JSONL 同为 13 行；全部行由 Python 标准 JSON 解析器独立解析成功，必需字段齐全。

`/status` 已输出真实字段：

```text
motion_enabled
motion_state
motion_score
motion_sample_fps
event_count
```

当前 `event_count` 是本次 daemon 运行期间通过 gate 的事件数；重启后从 0 开始，已有 JSONL 继续追加。该跨重启语义留到 M4 决定。

## Camera Missing 与恢复

真实拔插证据：

```text
usb 1-1.3: USB disconnect
uvcvideo: Failed to resubmit video URB (-19)
usb 1-1.3: new high-speed USB device
uvcvideo: Found UVC 1.00 device Web Camera
```

同一 daemon 进程保持存活，恢复后采集回到 30 FPS，motion sampling 回到 3 FPS。恢复后的第一个事件约在重新枚举 3.66 秒后产生，证明第一帧只建立 baseline，没有断开/恢复假事件。

## 30 分钟并行稳定性

测试时间：2026-07-26 17:59:31 至 18:29:36。负载为 UVC MJPEG 640x480@30、一个持续浏览器客户端和 3 FPS motion worker。每 5 分钟采样一次，共 7 个样本。

| 指标 | 起点 | 终点 / 范围 | 结果 |
| --- | ---: | ---: | --- |
| PID | 989 | 989 | 未重启 |
| frame_count | 10976 | 65110 | 持续增长 |
| stream FPS | 30.0 | 30.0-30.0 | Passed |
| motion sample FPS | 3.0 | 3.0-3.0 | Passed |
| client_count | 1 | 1-1 | Passed |
| CPU | 16.3% | 16.3%-16.6% | 稳定 |
| RSS | 2968 KB | 2968 KB，delta 0 | 无增长 |
| VSZ | 39028 KB | 39028 KB | 无增长 |
| threads | 4 | 4-4 | 稳定 |

相关 dmesg 前后没有新增 UVC、PXP、Oops、panic 或 segfault。事件数从 2 增至 8，新增事件集中在实际画面变化时；独立静止测试已经证明 5 分钟 `static_delta=0`。
## 计划验收

| 场景 | 预期 | 结果 |
| --- | --- | --- |
| 静止画面 | 不持续触发事件 | Passed：5 分钟 0 事件 |
| 明显挥手或物体移动 | 触发 motion event | Passed：10/10 |
| cooldown 内重复变化 | 不连续刷写事件 | Passed：持续动作 7 事件 |
| JSONL 格式 | 每行可独立解析 | Passed：13/13 行 |
| `/status` | motion 和 event count 为真实状态 | Passed |
| camera missing/recovery | 不崩溃、不产生恢复假事件 | Passed |
| MJPEG 性能 | 不低于 M2 基线约 90% | Passed：30.0 FPS |
| motion sampling | 稳定达到 2-5 FPS | Passed：3.0 FPS |
| 30 分钟稳定性 | 无崩溃和持续 RSS 增长 | Passed：PID 不变、RSS delta 0 |

M3 功能和验收已完成。下一步是完成提交与 PR，合入 `develop` 后创建 M4 分支处理 systemd 和故障注入。