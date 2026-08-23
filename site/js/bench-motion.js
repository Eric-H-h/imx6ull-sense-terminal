import { FACTS } from "./evidence-facts.js";
import { LAB } from "./lab-model.js";
import { rasterBlobFrame, attemptMotionEmit } from "./motion-math.js";

export function mountBenchMotion(store) {
  const waveBtn = document.getElementById("wave-btn");
  const idleBtn = document.getElementById("idle-btn");
  const pixelDelta = document.getElementById("pixel-delta");
  const ratio = document.getElementById("ratio");
  const cooldown = document.getElementById("cooldown");
  const sampleFps = document.getElementById("sample-fps");
  const note = document.getElementById("slider-note");
  const lens = document.getElementById("lens-canvas");
  const gray = document.getElementById("gray-canvas");
  const fill = document.getElementById("ratio-fill");
  const mark = document.getElementById("ratio-mark");
  const phase = document.getElementById("phase-label");
  const jsonl = document.getElementById("motion-jsonl");
  const lensCtx = lens.getContext("2d");
  const grayCtx = gray.getContext("2d", { willReadFrequently: true });

  let prev = null;
  let cx = 80;
  let dir = 1;
  let waving = false;
  let lastSampleAt = 0;

  if (note && !note.textContent.trim()) {
    note.innerHTML = `滑条是实验室范围；daemon 配置见 <a href="${FACTS.sources.configuration}">configuration.md</a>。`;
  }

  waveBtn.addEventListener("click", () => {
    waving = true;
  });
  idleBtn.addEventListener("click", () => {
    waving = false;
  });
  pixelDelta.addEventListener("input", () => {
    store.patch((s) => ({ ...s, motion: { ...s.motion, pixelDelta: Number(pixelDelta.value) } }));
  });
  ratio.addEventListener("input", () => {
    store.patch((s) => ({
      ...s,
      motion: { ...s.motion, changedRatioThreshold: Number(ratio.value) / 100 }
    }));
  });
  cooldown.addEventListener("input", () => {
    store.patch((s) => ({ ...s, motion: { ...s.motion, cooldownMs: Number(cooldown.value) } }));
  });
  sampleFps.addEventListener("input", () => {
    store.patch((s) => ({ ...s, motion: { ...s.motion, sampleFps: Number(sampleFps.value) } }));
  });

  store.subscribe((state) => {
    mark.style.left = `${state.motion.changedRatioThreshold * 100}%`;
    fill.style.width = `${Math.min(100, state.motion.score * 100)}%`;
    phase.textContent = `${state.motion.uiPhase.toUpperCase()}  score=${state.motion.score.toFixed(3)}`;
    jsonl.textContent = state.jsonl.lines.slice(-6).join("\n") || "（尚无 JSONL）";
    cooldown.title = state.motion.cooldownMs === 0 ? "板上拒绝" : "";
    draw(state);
  });

  function draw(state) {
    const w = FACTS.grayWidth;
    const h = FACTS.grayHeight;
    const cy = h / 2;
    const frame = rasterBlobFrame(w, h, cx, cy);
    lensCtx.fillStyle = "#0d100e";
    lensCtx.fillRect(0, 0, lens.width, lens.height);
    grayCtx.fillStyle = "#0d100e";
    grayCtx.fillRect(0, 0, gray.width, gray.height);
    const scaleX = lens.width / w;
    const scaleY = lens.height / h;
    const img = lensCtx.createImageData(w, h);
    for (let i = 0; i < frame.length; i += 1) {
      const g = frame[i];
      img.data[i * 4] = g;
      img.data[i * 4 + 1] = g;
      img.data[i * 4 + 2] = g;
      img.data[i * 4 + 3] = 255;
    }
    const tmp = document.createElement("canvas");
    tmp.width = w;
    tmp.height = h;
    tmp.getContext("2d").putImageData(img, 0, 0);
    lensCtx.imageSmoothingEnabled = false;
    lensCtx.drawImage(tmp, 0, 0, lens.width, lens.height);
    grayCtx.imageSmoothingEnabled = false;
    grayCtx.drawImage(tmp, 0, 0, gray.width, gray.height);
    if (prev) {
      const overlay = grayCtx.getImageData(0, 0, gray.width, gray.height);
      for (let y = 0; y < h; y += 1) {
        for (let x = 0; x < w; x += 1) {
          const i = y * w + x;
          const d = Math.abs(frame[i] - prev[i]);
          if (d >= state.motion.pixelDelta) {
            const gx = Math.floor(x * scaleX);
            const gy = Math.floor(y * scaleY);
            const p = (gy * gray.width + gx) * 4;
            overlay.data[p] = 217;
            overlay.data[p + 1] = 130;
            overlay.data[p + 2] = 24;
          }
        }
      }
      grayCtx.putImageData(overlay, 0, 0);
    }
    if (!state.camera.present) {
      lensCtx.fillStyle = "rgba(31,26,20,0.5)";
      lensCtx.fillRect(0, 0, lens.width, 28);
      lensCtx.fillStyle = "#fffdf6";
      lensCtx.fillText("摄像头不可用", 8, 18);
    }
  }

  return {
    sampleIfDue(state, modelMs) {
      if (state.chapter !== "motion" || !state.playing || !state.proc.alive || !state.camera.present) {
        return state;
      }
      const period = 1000 / state.motion.sampleFps;
      if (modelMs - lastSampleAt < period && lastSampleAt !== 0) {
        return state;
      }
      lastSampleAt = modelMs;
      const w = FACTS.grayWidth;
      const h = FACTS.grayHeight;
      if (waving) {
        cx += dir * LAB.blob.waveDxPerSample;
        if (cx < 40 || cx > w - 40) {
          dir *= -1;
        }
      }
      const curr = rasterBlobFrame(w, h, cx, h / 2);
      const emitted = attemptMotionEmit({
        prev,
        curr,
        width: w,
        height: h,
        pixelDelta: state.motion.pixelDelta,
        ratioThreshold: state.motion.changedRatioThreshold,
        gate: {
          phase: state.motion.gate,
          cooldownStartedMs: state.motion.cooldownStartedMs || 0,
          eventCount: state.jsonl.eventCountProcess
        },
        nowMs: modelMs,
        cooldownMs: state.motion.cooldownMs,
        sequence: state.slot.seq,
        writable: state.eventLog.writable
      });
      prev = curr;
      const lines = state.jsonl.lines.slice();
      if (emitted.wrote && emitted.line) {
        lines.push(emitted.line);
      }
      return {
        ...state,
        motion: {
          ...state.motion,
          score: emitted.score.changedRatio,
          changedPixels: emitted.score.changedPixels,
          totalPixels: emitted.score.totalPixels,
          gate: emitted.decision.phase,
          uiPhase: emitted.uiPhase,
          cooldownRemainingMs: emitted.decision.cooldownRemainingMs,
          cooldownStartedMs: emitted.decision.gate.cooldownStartedMs,
          baselineReady: !emitted.score.baselineCreated
        },
        jsonl: {
          ...state.jsonl,
          lines,
          eventCountProcess: emitted.decision.eventCount
        },
        status: {
          ...state.status,
          event_count: emitted.decision.eventCount,
          motion_state: emitted.score.motionDetected,
          event_log_state: emitted.writeFailed ? "unavailable" : state.status.event_log_state,
          health: emitted.writeFailed ? "degraded" : state.status.health,
          degraded: emitted.writeFailed || state.status.degraded,
          ok: !(emitted.writeFailed || state.status.degraded)
        }
      };
    }
  };
}
