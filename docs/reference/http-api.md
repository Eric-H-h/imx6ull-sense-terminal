# HTTP API 参考

当前 M2 daemon 提供三个只读 GET 接口。M2 验收完成前，协议稳定性仍以 [M2 evidence](../verification/evidence/M2_mjpeg_stream.md) 为准。

## `GET /`

返回内嵌 HTML 页面，页面使用 `<img src="/stream">` 显示摄像头画面。

## `GET /stream`

返回：

```http
Content-Type: multipart/x-mixed-replace; boundary=frame
```

后续每个 part 包含 `Content-Type: image/jpeg`、`Content-Length` 和完整 JPEG 字节。

## `GET /status`

返回 JSON：

```json
{
  "ok": true,
  "degraded": false,
  "device": "/dev/video1",
  "width": 640,
  "height": 480,
  "fps": 29.8,
  "frame_count": 1234,
  "client_count": 1,
  "motion_state": false,
  "event_count": 0,
  "last_error": null
}
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `ok` | 当前是否未处于 degraded |
| `degraded` | 摄像头采集是否处于降级状态 |
| `device` | 当前设备或最近尝试的设备 |
| `width`, `height` | 当前协商分辨率 |
| `fps` | 当前统计帧率 |
| `frame_count` | 已发布帧数 |
| `client_count` | 当前 MJPEG stream 客户端数 |
| `motion_state`, `event_count` | M3 前为占位值，不代表 motion 已实现 |
| `last_error` | 无错误时为 `null`，否则为可读错误文本 |

其他路径返回 404；非 GET 请求返回 405。当前服务不提供认证或 TLS，只用于可信局域网演示。
