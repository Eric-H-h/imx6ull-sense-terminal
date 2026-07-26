# 配置字段参考

配置文件示例位于 `config/config.json`。当前解析器只实现项目需要的有限 JSON 字段，不是通用 JSON 库。

| 字段 | 类型 | 默认值 | 当前作用 |
| --- | --- | --- | --- |
| `device` | string | `auto` | `auto` 动态扫描 UVC Capture 节点；也可指定节点 |
| `width` | integer | `640` | 请求采集宽度，范围 1-4096 |
| `height` | integer | `480` | 请求采集高度，范围 1-2160 |
| `fps_limit` | integer | `30` | 请求帧率，范围 1-120 |
| `jpeg_quality` | integer | `75` | 预留给软件 JPEG 编码；当前 MJPEG pass-through 不使用 |
| `http_port` | integer | `8080` | HTTP 监听端口，范围 1-65535 |
| `event_log` | string | `events.jsonl` | JSONL 事件追加路径；相对路径基于进程工作目录 |
| `motion_enabled` | boolean | `true` | 是否启动 motion worker |
| `motion_sample_fps` | integer | `3` | JPEG 抽样频率，范围 1-30 |
| `motion_jpeg_scale_denom` | integer | `4` | libjpeg 缩放分母，只允许 1、2、4、8 |
| `motion_pixel_delta_threshold` | integer | `25` | 单像素亮度绝对差阈值，范围 1-255 |
| `motion_changed_ratio_threshold` | number | `0.05` | 变化像素比例阈值，范围 `(0, 1]` |
| `motion_cooldown_ms` | integer | `1500` | 事件冷却时间，范围 1-3600000 ms |

当前示例：

```json
{
  "device": "auto",
  "width": 640,
  "height": 480,
  "fps_limit": 30,
  "jpeg_quality": 75,
  "http_port": 8080,
  "event_log": "events.jsonl",
  "motion_enabled": true,
  "motion_sample_fps": 3,
  "motion_jpeg_scale_denom": 4,
  "motion_pixel_delta_threshold": 25,
  "motion_changed_ratio_threshold": 0.05,
  "motion_cooldown_ms": 1500
}
```

解析器会拒绝错误类型、越界整数、非有限小数、空事件路径和不支持的 JPEG scale。当前参数由 M3 板端 benchmark 与运动校准确定，证据见 [M3 evidence](../verification/evidence/M3_motion_event.md)。
