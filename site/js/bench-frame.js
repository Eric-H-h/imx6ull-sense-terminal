import { FACTS } from "./evidence-facts.js";
import { sendDurationMs } from "./http-skip.js";

export function mountBenchFrame(store) {
  const play = document.getElementById("play-btn");
  const speed = document.getElementById("speed");
  const dropped = document.getElementById("show-dropped");
  const hint = document.getElementById("node-hint");
  const panel = document.getElementById("node-panel");
  const canvas = document.getElementById("ticket-canvas");
  const caption = document.getElementById("frame-caption");
  const ctx = canvas.getContext("2d");

  function fillCaption() {
    caption.innerHTML = `
      <p>这是模型，未连接开发板。</p>
      <p>AppState 只有一格；采集覆盖，不排队。</p>
      <p>慢客户端发送期间槽位已更新，下次 copy 最新序号，中间号跳过。</p>
      <p>采集在模型里仍是 ${FACTS.captureFps} FPS（板上单客户端 <a href="${FACTS.sources.m2}">${FACTS.captureFps}</a>）。卡片以约 ${FACTS.captureFps * 0.4} FPS 绘制，避免 30 张/秒不可读。</p>
      <p>HTTP 跳号按模型时间计算，不是按画出来的票据张数。把速度滑条拖向左侧，再勾选「显示丢弃的帧」。</p>
    `;
  }

  play.addEventListener("click", () => {
    store.patch((s) => ({ ...s, playing: !s.playing }));
  });
  play.addEventListener("keydown", (event) => {
    if (event.code === "Space") {
      event.preventDefault();
      play.click();
    }
  });
  speed.addEventListener("input", () => {
    const clientSpeed = Number(speed.value) / 100;
    store.patch((s) => ({
      ...s,
      http: { ...s.http, clientSpeed, sendDurationMs: sendDurationMs(clientSpeed) }
    }));
  });
  dropped.addEventListener("change", () => {
    store.patch((s) => ({ ...s, http: { ...s.http, showDropped: dropped.checked } }));
  });
  hint.addEventListener("click", () => {
    store.patch((s) => ({ ...s, camera: { ...s.camera, hintOpen: !s.camera.hintOpen } }));
  });

  store.subscribe((state) => {
    play.textContent = state.playing ? "暂停" : "播放";
    panel.hidden = !state.camera.hintOpen;
    if (state.camera.hintOpen) {
      panel.textContent = `扫描 /dev/video0..63
sysfs driver == uvcvideo？否则跳过（避开 pxp Oops）
VIDIOC_QUERYCAP：uvcvideo + Video Capture + Streaming
再协商 MJPG ${FACTS.width}×${FACTS.height}@${FACTS.captureFps}
节点号不是身份。详见 ${FACTS.sources.pxp}`;
    }
    fillCaption();
    paint(state);
  });

  function paint(state) {
    const w = canvas.width;
    const h = canvas.height;
    ctx.fillStyle = "#2b261f";
    ctx.fillRect(0, 0, w, h);
    ctx.fillStyle = "#d9d1c3";
    ctx.font = "13px system-ui";
    ctx.fillText("采集覆盖 →", 16, 28);
    ctx.fillText("HTTP copy / skip", 16, 132);
    ctx.fillText("最新一格（只有这一张）", 640, 28);

    const seq = state.slot.seq;
    const start = Math.max(0, seq - 7);
    for (let s = start; s <= seq; s += 1) {
      const i = s - start;
      const x = 16 + i * 72;
      const skipped = state.http.dropped.includes(s);
      if (skipped && !state.http.showDropped) {
        continue;
      }
      ctx.beginPath();
      ctx.fillStyle = skipped ? "#3d3830" : "#fffdf6";
      ctx.strokeStyle = skipped ? "#6b7280" : "#c45c26";
      ctx.setLineDash(skipped ? [5, 4] : []);
      ctx.rect(x, 42, 62, 44);
      ctx.fill();
      ctx.stroke();
      ctx.setLineDash([]);
      ctx.fillStyle = skipped ? "#9ca3af" : "#1f1a14";
      ctx.font = "bold 16px ui-serif, Georgia, serif";
      ctx.fillText(`#${s}`, x + 10, 70);
    }

    ctx.fillStyle = "#fffdf6";
    ctx.strokeStyle = "#c45c26";
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.rect(640, 42, 230, 150);
    ctx.fill();
    ctx.stroke();
    ctx.lineWidth = 1;
    ctx.fillStyle = "#c45c26";
    ctx.font = "12px system-ui";
    ctx.fillText("AppState slot", 656, 64);
    ctx.fillStyle = "#1f1a14";
    ctx.font = "bold 42px ui-serif, Georgia, serif";
    ctx.fillText(String(seq), 656, 118);
    ctx.font = "13px system-ui";
    ctx.fillStyle = "#6b7280";
    ctx.fillText(`sent ${state.http.lastSentSeq}  ·  skip ${state.http.dropped.length}`, 656, 150);
    ctx.fillText(state.playing ? "模型时钟在走" : "已暂停", 656, 172);
  }
}
