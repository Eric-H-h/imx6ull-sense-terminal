# M2 验收证据：MJPEG 浏览器推流

## 状态

- 状态：Completed
- 完成日期：2026-07-18
- 分支：`codex/m2-mjpeg-stream`
- 前置阶段：M0、M1 已完成
- 当前结论：UVC MJPEG 采集、HTTP 推流、浏览器连接生命周期、稳定性和正常退出均通过验证

## 验收对象

- V4L2 MJPEG MMAP 连续采集。
- 最新帧共享和并发访问。
- `GET /`、`GET /stream`、`GET /status`。
- 按 `uvcvideo` 预筛并避开已知 PXP 查询风险。
- 客户端刷新、断开和重连。
- 单客户端至少 30 分钟稳定性。
- SIGINT 正常退出和连接线程回收。

## 构建证据

主机提交前复跑：

```sh
make -C app/daemon clean all
file app/daemon/imx6ull-sense
```

关键输出：

```text
gcc -o imx6ull-sense main.o config.o state.o capture_v4l2.o http_server.o -pthread
app/daemon/imx6ull-sense: ELF 64-bit LSB pie executable, x86-64
host build exit: 0
```

ARM 交叉编译、SCP 传输和板端运行此前已由操作者完成，二进制与配置传输前后 SHA-256 一致，daemon 在 i.MX6ULL 上正常启动。2026-07-18 的自动复跑 shell 未继承操作者的交叉工具链 PATH，`arm-linux-gnueabihf-gcc` 返回 `No such file or directory`；这是当前自动执行环境差异，不推翻已经完成的 ARM 上板结果。

| 检查项 | 结果 | 关键证据 |
| --- | --- | --- |
| 主机干净构建 | Passed | GCC 完整编译和链接，exit 0，无告警 |
| ARM 交叉编译 | Passed（操作者验证） | ARM 产物已成功传到开发板并运行 |
| SCP 完整性 | Passed（操作者验证） | 二进制和配置传输前后 SHA-256 一致 |
| 板端启动 | Passed | 监听 `0.0.0.0:8080`，UVC MJPG 640x480@30 active |

## 板端启动

```sh
cd /tmp/imx6ull-sense-m2
./imx6ull-sense -c config.json
```

关键运行事实：

```text
imx6ull-sense listening on 0.0.0.0:8080
camera selector: auto, request: MJPG 640x480@30
capture active: /dev/video1 MJPG 640x480
```

设备编号只代表本次枚举结果。实现按 sysfs driver 和 V4L2 Device Caps 动态选择 `uvcvideo + Video Capture + Streaming` 节点，不把 `/dev/video1` 写成永久身份。

## HTTP 与 MJPEG 协议证据

WSL 中存在 SOCKS 代理时，访问板端私网地址必须绕过代理：

```sh
curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status
```

stream 响应头：

```text
HTTP/1.1 200 OK
Content-Type: multipart/x-mixed-replace; boundary=frame
Cache-Control: no-store, no-cache, must-revalidate
Pragma: no-cache
Connection: close
```

首个 multipart part 解析结果：

```text
boundary_count: 90
first_part_header:
--frame
Content-Type: image/jpeg
Content-Length: 25118
declared_length: 25118
captured_length: 25118
jpeg_soi: ff d8
jpeg_eoi: ff d9
first_frame_valid: True
```

`curl --max-time 3` 对无限 MJPEG 流返回 28 是测试端主动限时终止，不是服务错误。3 秒内记录 90 个 boundary，与请求的约 30 fps 一致。

| 检查项 | 结果 |
| --- | --- |
| `/` 页面 | Passed |
| `/status` JSON | Passed |
| multipart boundary | Passed |
| `Content-Length` 与 JPEG 长度一致 | Passed |
| JPEG SOI `ff d8` / EOI `ff d9` | Passed |

## 浏览器与连接行为

Windows 浏览器访问 `http://192.168.7.2:8080/`，操作者确认画面持续变化，刷新、关闭和重新打开均正常。

| 场景 | 结果 | 证据 |
| --- | --- | --- |
| 首次打开画面持续变化 | Passed | 浏览器人工确认 |
| 页面刷新 | Passed | 刷新后继续显示动态画面 |
| 关闭后重新打开 | Passed | 新连接成功 |
| 客户端断开后 daemon 继续运行 | Passed | daemon PID 保持不变 |
| 浏览器打开 | Passed | `client_count=1`，`Threads=3` |
| 浏览器关闭 | Passed | `client_count=0`，`Threads=2` |

## Degraded 与恢复

摄像头缺失、拔出和重新插入行为由操作者确认符合要求；本轮没有保留逐条原始输出，因此不补造具体 JSON 数值。M4 仍会进行正式 camera-missing 故障注入并保存完整日志。

| 场景 | 结果 | 证据形式 |
| --- | --- | --- |
| 启动时无摄像头 | Passed | 操作者确认 |
| 运行中拔出摄像头 | Passed | 操作者确认 |
| 重新插入自动恢复 | Passed | 操作者确认 |
| `/status` 与实际状态一致 | Passed | 操作者确认 |

## 稳定性

两次快照之间的实际运行时间约 41 分 30 秒，超过 30 分钟要求。

```text
开始：
PID 691, ELAPSED 43:38, CPU 1.9%, RSS 2996 kB, VSZ 30596 kB, Threads 2

结束：
PID 691, ELAPSED 01:25:08, CPU 6.7%, RSS 3000 kB, VSZ 30596 kB, Threads 2
```

| 指标 | 开始 | 结束 | 结论 |
| --- | ---: | ---: | --- |
| PID | 691 | 691 | 未崩溃、未重启 |
| CPU（进程累计平均） | 1.9% | 6.7% | 可接受 |
| RSS | 2996 kB | 3000 kB | 仅增加 4 kB，无失控增长 |
| VSZ | 30596 kB | 30596 kB | 稳定 |
| 线程数（无客户端快照） | 2 | 2 | 稳定 |
| 内核错误 | 无 UVC/PXP/panic 错误 | 仅有旧的 `dhd/wlan0` 断线重连日志 | 与 RNDIS/MJPEG 无关 |

## 正常退出

板端前台进程收到 `Ctrl+C` 后返回 shell，操作者确认退出码、进程消失和端口关闭均符合预期。串口终端复制包含正则方括号的 `grep '[i]mx6ull-sense'` 显示异常时，改用 `pidof imx6ull-sense` 完成确认。

## Bug / Blocker

- [M2-NET-PROXY-001](../../bug_reports/M2-NET-PROXY-001_wsl_proxy_bypasses_rndis.md)：WSL `curl` 被 SOCKS 代理接管，访问 RNDIS 私网地址超时；通过 `--noproxy` / `NO_PROXY` 解决。
- [M1-DRV-PXP-001](../../bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md)：M2 设备发现增加 sysfs driver 预筛，在执行 V4L2 ioctl 前跳过非 `uvcvideo` 节点。
- HTTP 连接增加 `SO_SNDTIMEO`，避免客户端停止读取时工作线程无限阻塞。该项是代码审查中的预防性修复，没有形成实际运行事故。

## M2 结论

M2 通过。浏览器 MJPEG 主链路、状态接口、客户端生命周期、至少 30 分钟稳定性和正常退出均满足验收要求。M3 只能在 M2 PR 合入 `develop` 后开始。
