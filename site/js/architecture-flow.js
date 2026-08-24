const SOURCE_BASE = "https://github.com/Eric-H-h/imx6ull-sense-terminal/blob/v0.1-mvp/app/daemon/";

const COPY = {
  camera: {
    label: "SOURCE",
    title: "USB UVC",
    summary: "动态发现真正提供视频帧的 UVC 节点，不依赖会变化的 /dev/videoX。",
    facts: [
      ["识别", "uvcvideo + Capture + Streaming"],
      ["输出", "可采集的 Video Capture 节点"],
      ["故障", "unavailable · 定期重试"]
    ],
    principle: "以驱动与能力识别设备，避免误选 PXP 或 Metadata 节点。",
    source: "capture_v4l2.c"
  },
  capture: {
    label: "CAPTURE",
    title: "V4L2 Capture",
    summary: "通过 MMAP 持续取得完整 MJPEG 帧，并发布为最新快照。",
    facts: [
      ["输入", "/dev/videoX · MJPEG"],
      ["输出", "Latest JPEG + sequence"],
      ["故障", "degraded · 关闭后重试"]
    ],
    principle: "采集不等待消费者，慢客户端不会拖住摄像头。",
    source: "capture_v4l2.c"
  },
  slot: {
    label: "SHARED STATE",
    title: "Latest JPEG",
    summary: "内存只保存一份完整 JPEG，并用 sequence 区分新旧。",
    facts: [
      ["写入者", "V4L2 Capture"],
      ["读取者", "HTTP Stream / Motion"],
      ["策略", "新帧覆盖旧帧 · 不排队"]
    ],
    principle: "有限资源平台优先保证低延迟和明确的内存上限。",
    source: "state.c"
  },
  http: {
    label: "LIVE DELIVERY",
    title: "HTTP MJPEG",
    summary: "客户端复制当前快照，通过 multipart 持续接收 JPEG。",
    facts: [
      ["输入", "Latest JPEG"],
      ["输出", "multipart/x-mixed-replace"],
      ["慢客户端", "跳过过期帧 · 不阻塞采集"]
    ],
    principle: "网络发送在共享锁外完成，慢连接只影响自身。",
    source: "http_server.c"
  },
  motion: {
    label: "LOW-RATE ANALYSIS",
    title: "Motion",
    summary: "约 3 FPS 抽样并缩放为 160 × 120 灰度图，再计算帧差。",
    facts: [
      ["采样", "Latest JPEG · 约 3 FPS"],
      ["输出", "motion score / event"],
      ["抑制", "threshold + cooldown"]
    ],
    principle: "首帧只建 baseline；低频检测控制 CPU 并避免启动误报。",
    source: "motion_detector.c"
  },
  jsonl: {
    label: "EVENT OUTPUT",
    title: "JSONL + /status",
    summary: "事件写入 JSONL；/status 同步暴露各组件健康状态。",
    facts: [
      ["输入", "confirmed motion event"],
      ["持久化", "每个事件一行 JSONL"],
      ["日志故障", "推流继续 · 恢复后追加"]
    ],
    principle: "日志故障必须可见，但不能带着视频服务一起退出。",
    source: "event_log.c"
  }
};

function renderFacts(facts) {
  return facts.map(([term, value]) => `<div><dt>${term}</dt><dd>${value}</dd></div>`).join("");
}

export function mountArchitecture(root) {
  const detail = document.getElementById("arch-detail");
  const nodes = [...root.querySelectorAll("[data-arch]")];

  function show(key) {
    const item = COPY[key];
    if (!item || !detail) {
      return;
    }

    nodes.forEach((node) => {
      node.setAttribute("data-active", node.getAttribute("data-arch") === key ? "1" : "0");
    });

    detail.dataset.tone = key;
    detail.innerHTML = `
      <div class="detail-head">
        <p class="detail-label">${item.label}</p>
        <h3>${item.title}</h3>
        <p class="detail-summary">${item.summary}</p>
      </div>
      <dl class="detail-facts">${renderFacts(item.facts)}</dl>
      <div class="detail-reason">
        <span>设计原则</span>
        <p>${item.principle}</p>
        <a class="detail-source" href="${SOURCE_BASE}${item.source}" target="_blank" rel="noreferrer">查看 ${item.source} 源码 →</a>
      </div>
    `;
  }

  nodes.forEach((node) => {
    const key = node.getAttribute("data-arch");
    node.addEventListener("click", () => show(key));
  });

  const canvas = root.querySelector(".arch-canvas");
  if (canvas) {
    mountArchWires(canvas);
  }
}

function mountArchWires(canvas) {
  const svg = canvas.querySelector(".arch-wires");
  if (!svg) {
    return;
  }
  const NS = "http://www.w3.org/2000/svg";

  function draw() {
    const rect = canvas.getBoundingClientRect();
    if (rect.width === 0 || rect.height === 0) {
      return;
    }
    const ox = rect.left + canvas.clientLeft;
    const oy = rect.top + canvas.clientTop;
    svg.setAttribute("viewBox", `0 0 ${canvas.clientWidth} ${canvas.clientHeight}`);

    const rel = (el) => {
      const r = el.getBoundingClientRect();
      return {
        left: r.left - ox,
        right: r.right - ox,
        top: r.top - oy,
        bottom: r.bottom - oy,
        cx: r.left - ox + r.width / 2,
        cy: r.top - oy + r.height / 2
      };
    };

    const d = [];

    canvas.querySelectorAll(".arch-arrow").forEach((arrow) => {
      const a = rel(arrow);
      const y = a.cy;
      const x2 = a.right - 2;
      d.push(`M ${a.left + 2} ${y} L ${x2} ${y}`);
      d.push(`M ${x2 - 5} ${y - 4} L ${x2} ${y} L ${x2 - 5} ${y + 4}`);
    });

    const slot = rel(canvas.querySelector('[data-arch="slot"]'));
    const httpNode = rel(canvas.querySelector('[data-arch="http"]'));
    const motionNode = rel(canvas.querySelector('[data-arch="motion"]'));
    const livePanel = rel(canvas.querySelector(".branch-live"));
    const motionPanel = rel(canvas.querySelector(".branch-motion"));

    const yBus = (slot.bottom + livePanel.top) / 2;
    const busX1 = Math.min(slot.cx, httpNode.cx, motionNode.cx);
    const busX2 = Math.max(slot.cx, httpNode.cx, motionNode.cx);

    d.push(`M ${slot.cx} ${slot.bottom} L ${slot.cx} ${yBus}`);
    d.push(`M ${busX1} ${yBus} L ${busX2} ${yBus}`);
    [[httpNode, livePanel], [motionNode, motionPanel]].forEach(([node, panel]) => {
      d.push(`M ${node.cx} ${yBus} L ${node.cx} ${panel.top}`);
      d.push(`M ${node.cx - 4} ${panel.top - 5} L ${node.cx} ${panel.top} L ${node.cx + 4} ${panel.top - 5}`);
    });

    let path = svg.querySelector("path");
    if (!path) {
      path = document.createElementNS(NS, "path");
      svg.appendChild(path);
    }
    path.setAttribute("d", d.join(" "));
  }

  draw();
  new ResizeObserver(draw).observe(canvas);
  if (document.fonts && document.fonts.ready) {
    document.fonts.ready.then(draw);
  }
  window.addEventListener("load", draw);
}
