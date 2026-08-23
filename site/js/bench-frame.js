import { FACTS } from "./evidence-facts.js";
import { sendDurationMs } from "./http-skip.js";

export function mountBenchFrame(store, options = {}) {
  const play = document.getElementById("play-btn");
  const reduceMotion = Boolean(options.reduced);
  const speed = document.getElementById("speed");
  const dropped = document.getElementById("show-dropped");
  const hint = document.getElementById("node-hint");
  const panel = document.getElementById("node-panel");
  const canvas = document.getElementById("ticket-canvas");
  const live = document.getElementById("frame-live");
  const ctx = canvas.getContext("2d");

  play.addEventListener("click", () => {
    if (reduceMotion) {
      if (typeof options.stepOnce === "function") {
        options.stepOnce(performance.now());
      }
      return;
    }
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
    play.textContent = reduceMotion ? "步进" : state.playing ? "暂停" : "播放";
    panel.hidden = !state.camera.hintOpen;
    if (state.camera.hintOpen) {
      panel.textContent = `扫描 /dev/video0..63
sysfs driver == uvcvideo？否则跳过（避开 pxp Oops）
VIDIOC_QUERYCAP：uvcvideo + Video Capture + Streaming
再协商 MJPG ${FACTS.width}×${FACTS.height}@${FACTS.captureFps}
节点号不是身份。详见 ${FACTS.sources.pxp}`;
    }
    if (live) {
      live.textContent = `seq ${state.slot.seq}，已发送 ${state.http.lastSentSeq}，跳过 ${state.http.dropped.length}`;
    }
    paint(state);
  });

  function paint(state) {
    const w = canvas.width;
    const h = canvas.height;
    ctx.fillStyle = "#0d100e";
    ctx.fillRect(0, 0, w, h);
    ctx.fillStyle = "#9aa89f";
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
      ctx.fillStyle = skipped ? "#1b221f" : "#f5f7f6";
      ctx.strokeStyle = skipped ? "#65716b" : "#2f6fdb";
      ctx.setLineDash(skipped ? [5, 4] : []);
      ctx.rect(x, 42, 62, 44);
      ctx.fill();
      ctx.stroke();
      ctx.setLineDash([]);
      ctx.fillStyle = skipped ? "#9aa89f" : "#121815";
      ctx.font = "bold 16px ui-serif, Georgia, serif";
      ctx.fillText(`#${s}`, x + 10, 70);
    }

    ctx.fillStyle = "#171d1a";
    ctx.strokeStyle = "#197255";
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.rect(640, 42, 230, 150);
    ctx.fill();
    ctx.stroke();
    ctx.lineWidth = 1;
    ctx.fillStyle = "#197255";
    ctx.font = "12px system-ui";
    ctx.fillText("AppState slot", 656, 64);
    ctx.fillStyle = "#f5f7f6";
    ctx.font = "bold 42px ui-serif, Georgia, serif";
    ctx.fillText(String(seq), 656, 118);
    ctx.font = "13px system-ui";
    ctx.fillStyle = "#9aa89f";
    ctx.fillText(`sent ${state.http.lastSentSeq}  ·  skip ${state.http.dropped.length}`, 656, 150);
    ctx.fillText(state.playing ? "模型时钟在走" : "已暂停", 656, 172);
  }
}
