# MJPEG over HTTP 原理

MJPEG stream 不是单个无限大的 JPEG，而是同一个 HTTP 响应中连续发送多个完整 JPEG。每一帧由 multipart boundary 分隔：

```http
HTTP/1.1 200 OK
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg
Content-Length: 12345

<JPEG bytes>
```

浏览器中的 `<img src="/stream">` 会持续消费后续 part 并刷新画面。

实现必须保证：

- 响应头中的 boundary 名称与帧前缀一致。
- 每帧 `Content-Length` 与实际 JPEG 字节数一致。
- header、JPEG 和换行顺序正确。
- 客户端断开不会终止整个进程。
- 慢客户端不会阻塞摄像头采集。

选择原因和约束见 [ADR-0002](../architecture/decisions/0002-use-mjpeg-over-http.md)，实际协议与稳定性结论以 [M2 证据](../verification/evidence/M2_mjpeg_stream.md) 为准。
