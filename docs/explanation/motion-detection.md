# Motion Detection 原理

## 当前实现

M3 使用可解释的低成本帧差，不使用 AI 模型，也不改变 M2 的 MJPEG 浏览器推流路线：

```text
latest MJPEG
  -> 3 FPS 抽样
  -> libjpeg 1/4 scaling
  -> 160x120 grayscale
  -> 与上一灰度帧逐像素比较
  -> changed_pixels / total_pixels
  -> threshold + cooldown
  -> JSONL event + /status
```

## 两层阈值

`motion_pixel_delta_threshold` 判断一个像素是否发生了足够明显的亮度变化。默认值 25，表示当前值与上一帧相差至少 25 才计入变化像素。

`motion_changed_ratio_threshold` 判断整张图变化像素的占比是否足以构成运动。默认值 0.05，即至少 5% 的 160x120 灰度像素发生明显变化。

这样设计比“所有像素差值相加”更容易解释，也能把局部噪声与明显运动分开。

## Baseline 与恢复

第一张灰度帧没有上一帧可比较，只用于建立 baseline，不产生事件。每次正常比较后，当前帧成为下一次 baseline。

摄像头缺失、采集超时或 capture generation 变化时，worker 清除当前 baseline。摄像头恢复后的第一帧同样只重建 baseline，因此不会把断开前后的整幅画面差异误认为运动。

## Cooldown

motion score 达到阈值时，状态机从 `IDLE` 进入 `COOLDOWN` 并写入一个事件。1500 ms 内即使画面持续变化，也不会立即重复写入；冷却结束后仍有运动才允许产生下一事件。

cooldown 限制的是事件频率，不会停止 score 计算或 `/status` 更新。

## JSONL

每个事件单独占一行 JSON，至少包含时间戳、帧序号、score、threshold、变化像素数、总像素数和 cooldown。单行格式便于追加写入、逐行解析和故障后保留已有记录。

当前 `event_count` 是本次进程运行期间的计数；daemon 重启后从 0 开始，而已有 JSONL 文件继续追加。跨重启计数语义留到 M4 决定。

## 参数依据

3 FPS、160x120、像素差 25、变化比例 5% 和 1500 ms cooldown 来自 i.MX6ULL 板端 benchmark 与现场校准。静止、10 次挥手、持续运动、浏览器并行、摄像头恢复和 30 分钟稳定性结果见 [M3 evidence](../verification/evidence/M3_motion_event.md)。
