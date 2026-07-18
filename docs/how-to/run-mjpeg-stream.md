# 运行和检查 MJPEG 服务

## 状态

M2 验收中。本页命令描述当前实现入口；只有写入 [M2 验收证据](../verification/evidence/M2_mjpeg_stream.md) 的结果才算已验证事实。

## 构建和部署

```sh
cd /home/eric/projects/imx6ull-sense-terminal
make -C app/daemon clean all CROSS_COMPILE=arm-linux-gnueabihf-
scp app/daemon/imx6ull-sense debian@192.168.7.2:/tmp/
scp config/config.json debian@192.168.7.2:/tmp/
```

板端运行：

```sh
ssh debian@192.168.7.2 '/tmp/imx6ull-sense -c /tmp/config.json'
```

## 检查接口

```sh
curl http://192.168.7.2:8080/status
curl -I http://192.168.7.2:8080/
curl --max-time 2 \
  -D /tmp/m2-stream.headers \
  -o /tmp/m2-stream.bin \
  http://192.168.7.2:8080/stream
```

浏览器打开：

```text
http://192.168.7.2:8080/
```

## 验收要求

- `/` 返回可用页面。
- `/status` 返回可解析 JSON，状态与摄像头实际情况一致。
- `/stream` 的 boundary、`Content-Length` 和 JPEG 字节正确。
- 页面画面持续变化。
- 刷新、关闭和重新连接后服务仍可用。
- 单客户端连续运行 30 分钟并记录 FPS、CPU、RSS 和内核日志。
