# 运行时与数据流

## 线程模型

当前 daemon 使用单进程、多线程结构：

```text
主线程
  ├─ 配置和 AppState 初始化
  ├─ 启动 capture thread
  └─ 运行 HTTP accept/poll 循环

capture thread
  └─ V4L2 DQBUF -> 复制 JPEG -> 发布最新帧 -> QBUF

HTTP client thread
  └─ 等待新帧 -> 复制最新 JPEG -> send multipart frame
```

每个 HTTP 连接由独立客户端线程处理。退出时主线程设置停止状态，唤醒等待者，并等待采集线程和客户端线程完成。

## 最新帧策略

AppState 只保存最新完整 JPEG 帧和递增序号：

- 采集速度不受慢客户端直接阻塞。
- 慢客户端跳过旧帧，不形成无界队列。
- 每个客户端发送前复制一份稳定帧，避免采集线程覆盖正在发送的数据。

代价是客户端不保证接收每一帧，且内存使用会随同时发送的客户端数量增加。

## 故障状态

摄像头打开、格式协商或采集失败时：

1. capture thread 更新 `degraded` 和 `last_error`。
2. HTTP 服务继续提供 `/status`。
3. 采集线程等待后重新扫描 UVC Capture 节点。
4. 意外致命错误和进程级恢复在 M4 交给 systemd 验证。

## 退出时序

```text
SIGINT/SIGTERM
  -> 设置 stop
  -> 唤醒 frame waiters
  -> 停止接收新连接
  -> capture cleanup
  -> 等待现有 client workers
  -> 销毁 AppState
```

M2 验收必须覆盖客户端刷新、关闭、重新连接和进程退出，实际结果记录到 [M2 证据](../verification/evidence/M2_mjpeg_stream.md)。
