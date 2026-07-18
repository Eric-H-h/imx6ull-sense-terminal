# M2 验收证据：MJPEG 浏览器推流

## 状态

- 状态：In Progress
- 分支：`codex/m2-mjpeg-stream`
- 前置阶段：M0、M1 已完成
- 当前结论：代码已实现，完整代码审查和板端验收尚未完成

## 验收对象

- V4L2 MJPEG MMAP 连续采集。
- 最新帧共享和并发访问。
- `GET /`、`GET /stream`、`GET /status`。
- 摄像头缺失 degraded 和自动重试。
- 客户端刷新、断开和重连。
- 单客户端 30 分钟稳定性。

## 构建证据

待执行并记录：

```sh
make -C app/daemon clean all
make -C app/daemon clean all CROSS_COMPILE=arm-linux-gnueabihf-
file app/daemon/imx6ull-sense
```

| 检查项 | 结果 | 关键输出 |
| --- | --- | --- |
| 主机干净构建 | Pending | |
| ARM 交叉编译 | Pending | |
| ARM 产物架构 | Pending | |

## 板端接口证据

```sh
curl http://192.168.7.2:8080/status
curl -I http://192.168.7.2:8080/
curl --max-time 2 \
  -D /tmp/m2-stream.headers \
  -o /tmp/m2-stream.bin \
  http://192.168.7.2:8080/stream
```

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| `/` 页面 | Pending | |
| `/status` JSON | Pending | |
| multipart boundary | Pending | |
| `Content-Length` | Pending | |
| JPEG SOI/EOI | Pending | |

## 浏览器与连接行为

| 场景 | 结果 | 证据 |
| --- | --- | --- |
| 首次打开画面持续变化 | Pending | |
| 页面刷新 | Pending | |
| 关闭后重新打开 | Pending | |
| 客户端断开后 daemon 继续运行 | Pending | |

## Degraded 与恢复

| 场景 | 结果 | 证据 |
| --- | --- | --- |
| 启动时无摄像头 | Pending | |
| 运行中拔出摄像头 | Pending | |
| 重新插入自动恢复 | Pending | |
| `/status` 与实际状态一致 | Pending | |

## 30 分钟稳定性

| 指标 | 开始 | 结束 | 结论 |
| --- | ---: | ---: | --- |
| frame count | | | Pending |
| FPS | | | Pending |
| CPU | | | Pending |
| RSS | | | Pending |
| 内核错误 | | | Pending |

## Bug / Blocker

当前尚未在本 evidence 中登记。代码审查或板端测试发现 meaningful bug 时，创建 `M2-<AREA>-NNN_<slug>.md` 并更新 [`bug_reports/README.md`](../../bug_reports/README.md)。

## M2 结论

Pending。以上必需项完成后才能创建 `docs/stage_summaries/M2_mjpeg_browser_stream.md` 并关闭 M2。
