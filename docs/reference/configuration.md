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
| `event_log` | string | `/var/log/imx6ull-sense/events.jsonl` | M3 事件日志目标路径；当前未写入 |
| `motion_threshold` | integer | `12000` | M3 motion 阈值；当前未使用 |
| `motion_cooldown_ms` | integer | `1500` | M3 事件冷却时间；当前未使用 |

当前示例：

```json
{
  "device": "auto",
  "width": 640,
  "height": 480,
  "fps_limit": 30,
  "jpeg_quality": 75,
  "http_port": 8080,
  "event_log": "/var/log/imx6ull-sense/events.jsonl",
  "motion_threshold": 12000,
  "motion_cooldown_ms": 1500
}
```

解析与范围检查仍处于 M2 代码审查范围；本文只描述当前源码接口，不替代板端验证。
