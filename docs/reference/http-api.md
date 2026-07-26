# HTTP API 参考

当前 daemon 提供三个只读 GET 接口。推流协议证据见 [M2 evidence](../verification/evidence/M2_mjpeg_stream.md)，motion 状态证据见 [M3 evidence](../verification/evidence/M3_motion_event.md)。

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
  "motion_enabled": true,
  "motion_state": false,
  "motion_score": 0.0125,
  "motion_sample_fps": 3.0,
  "event_count": 7,
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
| `motion_enabled` | motion worker 是否由配置启用 |
| `motion_state` | 最近一次有效抽样是否达到 motion 阈值 |
| `motion_score` | 最近一次有效抽样的变化像素比例，范围 0-1 |
| `motion_sample_fps` | worker 实际完成的抽样频率 |
| `event_count` | 本次 daemon 运行期间通过 cooldown gate 的事件数 |
| `last_error` | 无采集错误时为 `null`，否则为可读错误文本 |

其他路径返回 404；非 GET 请求返回 405。当前服务不提供认证或 TLS，只用于可信局域网演示。
