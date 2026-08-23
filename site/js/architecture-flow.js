const COPY = {
  camera: {
    title: "Camera",
    body: "USB UVC。按 uvcvideo + Video Capture + Streaming 选节点，不把 /dev/videoX 当身份，也不查询 PXP 输出节点。"
  },
  capture: {
    title: "V4L2 Capture",
    body: "MMAP 采集 MJPEG。失败时更新 camera_state 并重试，进程不退出。"
  },
  slot: {
    title: "Latest JPEG",
    body: "AppState 只保留最新一格完整 JPEG 和 sequence。新帧覆盖，无历史队列。"
  },
  http: {
    title: "HTTP MJPEG",
    body: "客户端复制当前格再发送。发送期间采集继续；下次等到更新的 sequence，中间号跳过。"
  },
  motion: {
    title: "Motion",
    body: "约 3 FPS 抽样，160×120 灰度帧差。第一帧只建 baseline。"
  },
  jsonl: {
    title: "JSONL + /status",
    body: "过阈值且非 cooldown 则追加 JSONL。/status 输出真实 health、camera、event_log 与布尔 motion_state。"
  }
};

export function mountArchitecture(root) {
  const detail = document.getElementById("arch-detail");
  const nodes = [...root.querySelectorAll("[data-arch]")];

  function show(key) {
    const item = COPY[key];
    if (!item || !detail) {
      return;
    }
    nodes.forEach((n) => n.setAttribute("data-active", n.getAttribute("data-arch") === key ? "1" : "0"));
    detail.innerHTML = `<h3>${item.title}</h3><p>${item.body}</p>`;
  }

  nodes.forEach((node) => {
    const key = node.getAttribute("data-arch");
    node.addEventListener("click", () => show(key));
    node.addEventListener("keydown", (event) => {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        show(key);
      }
    });
  });
}
