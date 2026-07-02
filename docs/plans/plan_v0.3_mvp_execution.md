# i.MX6ULL Sense Terminal 项目完整规划 v0.3

## 1. 项目定义与实现目标

本项目要完成一个运行在野火 EBF6ULL S1 Pro / i.MX6ULL eMMC 开发板上的嵌入式 Linux 智能感知服务。它不是大型视频监控平台，而是一个小而完整、能上板验证、能浏览器演示、能写进简历并经得起面试追问的工程闭环。

最终实现链路：

```text
V4L2 摄像头采集
  -> JPEG/MJPEG 图像转换
  -> 浏览器实时预览
  -> 简单运动检测
  -> 本地 JSONL 事件日志
  -> systemd 长期运行和自动恢复
  -> 故障注入与测试报告
```

核心目标：

- 在 i.MX6ULL 板端真实运行，而不是只写主机代码。
- 浏览器打开板端 IP 即可看到实时画面。
- 挥手或画面变化能触发 motion event。
- 摄像头异常、配置错误、进程崩溃都有可解释行为。
- README、测试报告、面试问答能说明“我实现了什么、为什么这样设计、如何验证”。

当前执行分支默认为 `develop`，主 session 已切到 unborn `develop`，第一次提交会落在 `develop` 上。

摄像头路线调整为：MVP 第一阶段先使用 USB UVC 摄像头打通完整软件链路；OV5640 作为时间允许时的增强项。这样先保证项目能在 7 月底前形成可演示闭环，再用 OV5640 补充更贴近板级硬件接口的内容。

## 2. 项目原理

### 2.1 摄像头采集原理

Linux 摄像头设备通过 V4L2 暴露为 `/dev/videoX`。项目使用 V4L2 ioctl 完成设备打开、能力查询、格式设置、buffer 申请、mmap、入队、启动采集、出队取帧、重新入队。

MVP 默认先使用 USB UVC 摄像头。UVC 摄像头内部已经处理传感器、ISP 和 USB 传输，Linux 侧通常直接出现 `/dev/videoX`，适合优先打通：

```text
USB UVC 摄像头 -> Linux UVC driver -> /dev/videoX -> V4L2 userspace daemon
```

这条路线仍然保留 V4L2、MJPEG、motion、systemd、故障注入等核心能力，同时显著降低设备树、传感器供电、MCLK、PWDN/RESET 和 CSI pinout 的风险。

默认优先采集 YUYV；如果 UVC 摄像头直接稳定输出 MJPEG，也允许先走 MJPEG pass-through：

```text
/dev/videoX -> V4L2 mmap buffers -> YUYV frame
```

YUYV 的优势：

- Y 分量就是亮度，适合做简单运动检测。
- 数据结构直接，可解释性强。
- 不依赖复杂视频编解码器。

OV5640 不再作为 MVP 默认阻塞项。等 UVC 链路完成后，如果时间允许，再尝试适配明确支持 EBF6ULL/i.MX6ULL 的 OV5640 DVP/并口 CSI 模组：

```text
OV5640 DVP/CSI -> i.MX6ULL CSI -> V4L2 -> 同一套 userspace daemon
```

OV5640 的价值是展示板级摄像头链路；风险是可能涉及设备树、驱动、时钟、电源和排线 pinout 调试。

### 2.2 MJPEG 浏览器预览原理

浏览器原生能显示 MJPEG over HTTP。服务端持续输出 JPEG 帧，并用 multipart 边界分隔：

```http
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg
Content-Length: ...

<JPEG bytes>
--frame
...
```

本项目不做 H.264：

- i.MX6ULL 没有适合本项目主线的硬件 H.264 编码路径。
- 软件 H.264 对 Cortex-A7 太重。
- MJPEG 虽然带宽更高，但 LAN 演示足够，且实现简单、可解释。

### 2.3 运动检测原理

运动检测采用 Y 帧差：

```text
current Y frame - previous/reference Y frame -> changed pixels/score -> threshold -> event
```

具体逻辑：

- 从 YUYV 中抽取 Y 分量。
- 与上一帧或参考帧逐点比较。
- 差值超过像素阈值的点累计为 motion score。
- `score >= motion_threshold` 时触发事件。
- 使用 `motion_cooldown_ms` 防止连续刷屏。
- 事件写入 JSONL，便于 `tail -f`、测试和面试展示。

不实现录像、区域检测、人形识别、AI 模型。

### 2.4 systemd 可靠性原理

服务作为 Linux daemon 由 systemd 管理：

- `Restart=always` 或等价策略保证异常退出后恢复。
- `journalctl` 记录 stdout/stderr。
- `systemctl enable` 验证开机自启。
- 通过 `kill -9`、拔摄像头、坏配置做故障注入。

项目价值不只是“能跑”，而是能证明嵌入式服务的长期运行和异常处理能力。

## 3. 模块设计

### 3.1 `main`

职责：

- 解析命令行参数：`./imx6ull-sense -c config.json`。
- 加载配置。
- 初始化全局状态。
- 启动 capture、HTTP、motion/event 相关模块。
- 处理退出信号，释放资源。

### 3.2 `config_loader`

职责：

- 读取 JSON 配置。
- 提供设备路径、分辨率、fps、JPEG quality、HTTP port、事件日志路径、motion 阈值和 cooldown。
- 坏配置时 fail-safe：输出明确错误并退出非零，不用错误参数继续运行。

沿用当前配置字段：

```json
{
  "device": "/dev/video0",
  "width": 640,
  "height": 480,
  "fps_limit": 15,
  "jpeg_quality": 75,
  "http_port": 8080,
  "event_log": "/var/log/imx6ull-sense/events.jsonl",
  "motion_threshold": 12000,
  "motion_cooldown_ms": 1500
}
```

### 3.3 `capture_v4l2`

职责：

- 打开 `/dev/videoX`。
- 查询 capability。
- 设置格式，默认优先 YUYV。
- 申请 mmap buffer。
- 循环取帧。
- 将最新原始帧交给 frame buffer。
- 摄像头不存在或初始化失败时进入 degraded 状态，不让整个服务无解释崩溃。

默认起步参数：

```text
320x240
10 fps
YUYV preferred
JPEG quality 70-75
```

如果性能稳定，再提升到 `640x480@15fps`。

### 3.4 `frame_buffer`

职责：

- 保存最新一帧原始帧或 JPEG 帧。
- 维护 `frame_count`、时间戳、宽高、格式。
- 用 mutex/condition variable 保护多线程读写。
- HTTP 客户端只读最新帧，不排队历史帧，避免内存膨胀。

### 3.5 `jpeg_encoder`

职责：

- 将 YUYV/RGB 数据编码成 JPEG。
- 第一版使用系统 libjpeg 或 libjpeg-turbo 开发包。
- 如果摄像头直接输出 MJPEG，可先 pass-through，减少 CPU 压力。
- 性能不足时按顺序降级：分辨率、fps、quality。

### 3.6 `http_server`

职责：

- 监听配置中的 `http_port`。
- 实现三个最小接口：
  - `GET /`：返回 HTML 页面，内嵌 `<img src="/stream">`。
  - `GET /stream`：返回 multipart MJPEG。
  - `GET /status`：返回 JSON 状态。
- 只保证少量客户端，MVP 验收以单客户端 30 分钟稳定为准。
- 不做登录认证、不做复杂路由、不做 Web UI 框架。

`/status` 最小字段：

```json
{
  "ok": true,
  "degraded": false,
  "device": "/dev/video0",
  "width": 320,
  "height": 240,
  "fps": 9.8,
  "frame_count": 1234,
  "client_count": 1,
  "motion_state": false,
  "event_count": 3,
  "last_error": null
}
```

### 3.7 `motion_detector`

职责：

- 从 YUYV 帧中提取 Y 分量。
- 计算帧差 score。
- 使用 `motion_threshold` 判断是否触发。
- 使用 `motion_cooldown_ms` 控制事件频率。
- 更新全局 `motion_state` 和 `event_count`。

### 3.8 `event_log`

职责：

- 将运动事件追加写入 JSONL。
- 日志路径来自配置，例如 `/var/log/imx6ull-sense/events.jsonl`。
- 每行一个事件，便于 shell 工具查看。

事件格式：

```json
{"ts":"2026-07-20T20:10:11+08:00","type":"motion","score":14231,"threshold":12000,"cooldown_ms":1500}
```

### 3.9 `status`

职责：

- 统一维护运行状态。
- 提供给 HTTP `/status`。
- 记录 degraded、last_error、fps、frame_count、client_count、motion_state、event_count。
- 摄像头失败、配置失败、编码失败都要有可读错误信息。

### 3.10 `systemd`

职责：

- 安装服务文件。
- 管理启动、重启、日志、开机自启。
- 验证 kill/reboot/camera missing/bad config。

默认部署路径：

```text
/usr/local/bin/imx6ull-sense
/etc/imx6ull-sense/config.json
/var/lib/imx6ull-sense
/var/log/imx6ull-sense/events.jsonl
```

## 4. 详细实施计划

### M0: 环境与板端基线

目标：先证明开发环境和板端闭环可用，不写 V4L2 代码。

当前状态：

- WSL 项目路径：`/home/eric/projects/imx6ull-sense-terminal`
- Git 可用：`git version 2.43.0`
- `make/gcc/cc/clang` 缺失
- `sudo -n true` 需要密码
- `docs/01_bringup.md` 已记录当前阻塞
- 分支已切到 unborn `develop`

执行步骤：

1. 用户在 WSL 中安装工具链：

   ```sh
   cd /home/eric/projects/imx6ull-sense-terminal
   sudo apt update
   sudo apt install -y build-essential git
   ```

2. 主 session 验证：

   ```sh
   make --version
   gcc --version
   git --version
   make -C app/daemon
   ./app/daemon/imx6ull-sense -c config/config.json
   ```

3. 用户配置 Git 身份：

   ```sh
   git config user.name "侯子豪"
   git config user.email "用户自己的 GitHub 邮箱或 noreply 邮箱"
   ```

4. 板端执行并记录：

   ```sh
   uname -a
   cat /proc/cpuinfo
   ip addr
   df -h
   dmesg | grep -iE "error|fail|timeout|panic|mmc"
   ```

5. 确认传输路径：

   ```sh
   scp app/daemon/imx6ull-sense root@BOARD_IP:/usr/local/bin/
   ```

M0 验收：

- WSL 能构建并运行 scaffold。
- 板子能登录。
- PC 到板子的文件传输路径明确。
- `docs/01_bringup.md` 有真实命令和输出。
- 首个提交可在 `develop` 上完成。

### M1: USB UVC 摄像头 bring-up 与一帧采集

目标：先用 USB UVC 摄像头证明至少一个 `/dev/videoX` 能真实输出图像帧，打通应用层视频链路的入口。

执行步骤：

1. 准备一个 Linux 免驱 USB UVC 摄像头，优先选择支持 MJPEG/YUYV 的 720P 普通摄像头。
2. 板端准备 `v4l2-ctl`。
3. 插入 UVC 摄像头后枚举设备：

   ```sh
   v4l2-ctl --list-devices
   ```

4. 查看格式：

   ```sh
   v4l2-ctl -d /dev/videoX --list-formats-ext
   ```

5. 优先选择能稳定工作的低规格格式，例如 `320x240@10fps` 或 `640x480@10fps`。
6. 如果设备支持 MJPEG，先记录 MJPEG 格式；如果支持 YUYV，也记录 YUYV 格式，用于后续 motion 的 Y 分量处理。
7. 使用 v4l2 工具抓取一帧，记录输出文件、格式、大小。
8. 再实现最小 `capture_v4l2`：只负责打开 UVC 设备、设置格式、取一帧或连续取帧并统计 frame_count。
9. UVC 链路成功后，把 OV5640 放入可选增强任务；只有在剩余时间充足且模组明确适配 EBF6ULL/i.MX6ULL 时再执行。

M1 验收：

- 至少一个 `/dev/videoX` 可工作。
- `docs/02_camera_capture.md` 记录设备、格式、命令、输出。
- 有一帧真实图片或原始帧证据。
- 代码层可以取到帧，但还不要求浏览器显示。
- OV5640 不再是 M1 验收条件。

### M2: MJPEG 浏览器实时流

目标：浏览器访问板端 HTTP 服务能看到实时画面。

执行步骤：

1. 实现 frame buffer，保存最新帧。
2. 实现 JPEG 编码：
   - YUYV 输入：编码 JPEG。
   - MJPEG 输入：允许 pass-through。
3. 实现 HTTP server：
   - `GET /`
   - `GET /stream`
   - `GET /status`
4. capture 线程持续更新最新帧。
5. HTTP stream 线程不断发送最新 JPEG。
6. 记录单客户端 30 分钟稳定性、fps、CPU 粗略观察。

M2 验收：

- 浏览器能打开 `http://BOARD_IP:PORT/`。
- 画面能持续刷新。
- `curl http://BOARD_IP:PORT/status` 返回 JSON。
- `docs/03_mjpeg_stream.md` 有 fps、CPU、稳定性记录。

### M3: 运动事件检测

目标：让项目从“视频流”变成“感知服务”。

执行步骤：

1. 从采集帧中提取 Y 分量。
2. 实现 frame diff score。
3. 加入 `motion_threshold` 判断。
4. 加入 cooldown，避免重复刷事件。
5. 写 JSONL event log。
6. `/status` 增加 `motion_state` 和 `event_count`。
7. 用挥手、静止画面、快速遮挡测试。

M3 验收：

- 静止画面不持续触发。
- 挥手能触发 motion。
- cooldown 生效。
- JSONL 日志可读。
- `docs/04_motion_event.md` 有测试命令和输出。

### M4: systemd 服务化与故障注入

目标：证明它是一个可靠嵌入式 Linux 服务。

执行步骤：

1. 安装二进制和配置：

   ```sh
   install -m 755 imx6ull-sense /usr/local/bin/
   mkdir -p /etc/imx6ull-sense /var/lib/imx6ull-sense /var/log/imx6ull-sense
   ```

2. 安装 systemd unit。
3. 验证：

   ```sh
   systemctl daemon-reload
   systemctl enable imx6ull-sense
   systemctl start imx6ull-sense
   systemctl status imx6ull-sense
   journalctl -u imx6ull-sense -f
   ```

4. 故障注入：
   - `kill -9` 主进程，验证自动恢复。
   - reboot，验证开机自启。
   - 摄像头缺失，验证 degraded。
   - 配置损坏，验证 fail-safe。
5. 更新测试报告。

M4 验收：

- systemd 服务能启动和重启。
- kill 后能恢复。
- reboot 后能自启。
- 摄像头缺失和坏配置都有明确行为。
- `docs/05_fault_injection.md` 与 `docs/test_report.md` 完整。

### M5: 简历与演示包装

目标：让项目能被快速理解和展示。

执行步骤：

1. README 写清：
   - 项目目标
   - 架构图
   - 编译部署
   - HTTP 接口
   - 测试结果
   - 故障注入结果
2. `docs/interview_qa.md` 补充面试问答：
   - 为什么用 MJPEG？
   - 为什么不直接用 uStreamer？
   - V4L2 mmap 流程是什么？
   - motion 如何实现？
   - systemd 如何验证可靠性？
3. 准备 demo 流程：
   - 启动服务
   - 浏览器看画面
   - 挥手触发事件
   - 查看 JSONL
   - kill 进程看恢复

M5 验收：

- README 可独立说明项目。
- 面试问答覆盖关键技术点。
- demo 能在 3-5 分钟内跑完。

## 5. 范围控制与 fallback

硬优先级：

1. 真实板端运行。
2. 浏览器可见视频流。
3. motion event 日志。
4. systemd 和故障注入证据。
5. README / 简历包装。

fallback 规则：

- USB UVC 是 MVP 默认路线；如果某个 UVC 摄像头不兼容，直接换另一个 Linux 免驱 UVC 摄像头。
- OV5640 作为增强项；只有 UVC 闭环完成后才投入，若两个晚上不通就停止，不影响 MVP。
- HDMI 卡住一个晚上，放弃 HDMI。
- JPEG 太慢，降到 `320x240@5-10fps`。
- systemd 临时卡住，可短期用 shell watchdog 记录现象，但最终必须回 systemd。
- M2 未完成前，不做 M3/M4 扩展。
- MVP 前不做 AI、录像、OTA、字符驱动、复杂 Web UI。

## 6. 测试与记录规范

每次工作 session 结束必须记录：

```text
Attempted: 做了什么
Command: 跑了什么命令
Output: 看到什么输出
Success: 是否满足验收
Next: 下一步唯一动作
```

关键测试矩阵：

| 阶段 | 测试 | 成功标准 |
|---|---|---|
| M0 | WSL build | scaffold 能编译运行 |
| M0 | 板端登录 | 能执行基础命令 |
| M0 | 文件传输 | PC 能复制二进制到板端 |
| M1 | v4l2 list | 能看到 `/dev/videoX` 和格式 |
| M1 | 抓一帧 | 有真实帧文件 |
| M2 | `/` | 浏览器能打开页面 |
| M2 | `/stream` | 画面持续刷新 |
| M2 | `/status` | JSON 状态正确 |
| M3 | 静止 | 不持续触发 motion |
| M3 | 挥手 | 触发 event |
| M4 | kill -9 | systemd 自动恢复 |
| M4 | reboot | 服务自启动 |
| M4 | camera missing | degraded 可见 |
| M4 | bad config | fail-safe 可见 |

## 7. 协作规则

当前 session：

- 负责项目规划、路线评审、风险判断、fallback 决策。
- 负责把规划变化及时同步给主 session。
- 后续持续迭代本计划文档。

主 session `019f18cd-109c-7f00-9f4d-420042465647`：

- 负责实际执行。
- 默认在 `develop` 分支推进。
- M0 未完成前，不写 V4L2/MJPEG 实现。

执行边界：

- 不猜用户 Git 邮箱。
- 不复制 GPL 项目源码。
- 不静默切分支；需要切分支时先说明。
- 每个 milestone 完成后同步真实日志，再进入下一阶段。

## 8. Git 分支、Commit、MR/PR 规则

### 8.1 当前阶段分支策略

当前仓库仍处于 `No commits yet on develop` 状态，`develop` 是 unborn branch。因此当前阶段保持在 `develop`，不要现在再切新的 feature 分支；先让第一次提交落在 `develop` 上。

初始提交完成后：

- `develop` 作为日常集成分支。
- `main` 只用于最终稳定 MVP/release。
- 每个里程碑开始前，从 `develop` 切一个独立工作分支。
- Codex 创建分支时统一使用 `codex/` 前缀。

建议分支：

```text
develop
codex/m0-bringup
codex/m1-uvc-capture
codex/m2-mjpeg-stream
codex/m3-motion-event
codex/m4-systemd-fault
codex/m5-docs-demo
main
```

切分支时机：

- 在开始某个里程碑的代码/文档改动前切分支。
- 不要在已经改了一半后再切分支。
- 如果工作区有无关脏改动，先停下说明，不静默切分支。
- 小范围计划文档调整可以留在当前规划上下文；实际代码实现必须走对应里程碑分支。

### 8.2 Commit 规则

第一次提交：

- 等 M0 至少完成 WSL 构建工具安装、scaffold 可 build/run、`docs/01_bringup.md` 记录真实日志后提交。
- 不猜用户邮箱；用户配置好 `git config user.email` 后再 commit。
- 建议提交信息：

```text
chore: initialize imx6ull sense terminal project
```

后续提交节奏：

- 不等整个项目做完再提交。
- 每完成一个可验证的小闭环就提交。
- 每个 commit 必须对应能说明的结果：命令、输出、日志或文档证据。
- 不把未验证代码和真实测试日志混在一个大 commit 里。

建议 commit 类型：

```text
docs: record m0 bring-up logs
docs: record uvc camera formats
feat: add v4l2 capture loop
feat: add mjpeg http stream
feat: add motion event log
chore: add systemd service validation
docs: add final test report
```

### 8.3 MR/PR 规则

这里的 MR/PR 统一指“工作分支合并回 `develop` 的请求”。如果使用 GitHub，就叫 PR；如果使用 GitLab，就叫 MR。

什么时候开 MR/PR：

- 一个里程碑达到验收标准后开。
- 相关 docs 已经写入真实命令和输出后开。
- 本地 `git status` 确认只包含本里程碑相关变更后开。
- 不为每个小 commit 单独开 MR/PR。

建议 MR/PR 粒度：

```text
MR/PR 1: M0 bring-up and initial environment baseline
MR/PR 2: M1 USB UVC camera capture baseline
MR/PR 3: M2 MJPEG browser stream
MR/PR 4: M3 motion event logging
MR/PR 5: M4 systemd and fault injection
MR/PR 6: M5 README, test report, interview packaging
```

什么时候不开 MR/PR：

- M0 构建工具还没装好时不开。
- 没有真实测试日志时不开。
- 代码还不能编译时不开。
- 板端关键命令没跑过时不开。
- 工作区混入无关改动时不开。

MR/PR 描述模板：

```md
## Summary
- 本次完成的里程碑：
- 本次实现/更新内容：

## Verification
- Command:
- Output:
- Result:

## Docs Updated
- docs/...

## Known Issues
- 当前阻塞：
- 下一步：
```

### 8.4 Release Flow

MVP 前：

- 所有开发合并到 `develop`。
- `main` 不作为日常开发分支。
- `develop` 可以不完美，但每次 MR/PR 合并后应保持可构建、可解释。

MVP 完成后：

- 当 M0-M4 全部完成，M5 文档包装完成后，从 `develop` 发 release MR/PR 到 `main`。
- release 合并后打 tag：

```text
v0.1-mvp
```

release 条件：

- 浏览器能看到 MJPEG stream。
- motion event 能写 JSONL。
- systemd restart/reboot/camera missing/bad config 都有测试记录。
- README 和 `docs/test_report.md` 能独立说明项目结果。

### 8.5 Git 工作流 Assumptions

- 当前默认执行分支是 `develop`。
- 当前还没有 remote；remote 配好后再 push 和开 MR/PR。
- GitHub 场景下使用 PR，GitLab 场景下使用 MR，规则相同。
- 主 session 不自动乱切分支；需要切分支时先说明当前状态和目标分支。
- M0 未完成前，不开 M1/M2 的实现分支，不写 V4L2/MJPEG 代码。

## 9. Assumptions

- DDL 是 2026 年 7 月底前完成可演示 MVP。
- 用户平日晚上约 2 小时，周末可多推进。
- 第一目标是“板端闭环和可讲清楚”，不是功能数量。
- 默认使用 C 实现 daemon。
- 默认 HTTP server 自己实现最小 socket 服务，不引入大型 Web 框架。
- 默认 motion 使用 Y 帧差，不引入 OpenCV。
- 默认事件日志用 JSONL，不引入数据库。
- 默认 first stable config 为 `320x240@10fps, quality 70-75`。
- 默认摄像头路线为 USB UVC first；OV5640 是可选增强项，不阻塞 MVP。
