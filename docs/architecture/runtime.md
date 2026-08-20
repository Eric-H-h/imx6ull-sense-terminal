# 运行时与数据流

## 线程模型

当前 daemon 使用单进程、多线程结构：

```text
主线程
  ├─ 配置和 AppState 初始化
  ├─ 启动 capture thread
  ├─ 启动 motion worker thread
  └─ 运行 HTTP accept/poll 循环

capture thread
  └─ V4L2 DQBUF -> 复制 JPEG -> 发布最新帧 -> QBUF

motion worker thread
  └─ 3 FPS 等待最新 JPEG -> 锁外灰度解码 -> 帧差
     -> cooldown -> JSONL -> 更新 motion 状态

HTTP client thread
  └─ 等待新帧 -> 复制最新 JPEG -> send multipart frame
```

每个 HTTP 连接由独立客户端线程处理。退出时主线程设置停止状态，唤醒等待者，并等待采集、motion 和客户端线程完成。

## 最新帧策略

AppState 只保存最新完整 JPEG 帧和递增序号：

- 采集速度不受慢客户端直接阻塞。
- 慢客户端跳过旧帧，不形成无界队列。
- 每个客户端发送前复制一份稳定帧，避免采集线程覆盖正在发送的数据。

代价是客户端不保证接收每一帧，且内存使用会随同时发送的客户端数量增加。

## Motion 抽样策略

motion worker 只在计划采样点复制最新 JPEG，随后在 AppState 锁外执行 libjpeg 解码和帧差，因此不会长时间阻塞采集线程或 HTTP 客户端。摄像头缺失、超时或 capture generation 变化时会清除当前 motion 状态并重建 baseline；恢复后的第一帧不产生事件。

当前默认参数是 3 FPS、libjpeg `1/4` scaling、像素差阈值 25、变化比例阈值 5% 和 1500 ms cooldown。参数来源见 [M3 evidence](../verification/evidence/M3_motion_event.md)。

## 故障状态

摄像头打开、格式协商或采集失败时：

1. capture thread 更新 `degraded` 和 `last_error`。
2. HTTP 服务继续提供 `/status`。
3. 采集线程等待后重新扫描 UVC Capture 节点。
4. 进程被杀死后由 systemd `Restart=on-failure` 拉起；配置错误以退出码 78 停止，不自动重启。

motion worker 在 degraded 期间不产生事件；恢复后的第一张有效灰度帧只建立 baseline。

## 退出时序

```text
SIGINT/SIGTERM
  -> 设置 stop
  -> 唤醒 frame waiters
  -> 停止接收新连接
  -> capture cleanup
  -> motion pipeline cleanup
  -> 等待现有 client workers
  -> 销毁 AppState
```

M2 推流结果记录在 [M2 evidence](../verification/evidence/M2_mjpeg_stream.md)，M3 motion 与恢复结果记录在 [M3 evidence](../verification/evidence/M3_motion_event.md)，M4 服务与故障注入记录在 [M4 evidence](../verification/evidence/M4_fault_injection.md)。
