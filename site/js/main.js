import { FACTS } from "./evidence-facts.js";
import { LAB } from "./lab-model.js";
import { sendDurationMs, skippedSeqs } from "./http-skip.js";
import { createStore } from "./store.js";
import { createClock } from "./clock.js";
import { tick as faultTick, seedDemoLine } from "./fault-machine.js";
import { mountCrossSection } from "./cross-section.js";
import { mountBenchFrame } from "./bench-frame.js";
import { mountBenchMotion } from "./bench-motion.js";
import { mountBenchFault } from "./bench-fault.js";

function initialState() {
  return {
    chapter: "frame",
    playing: true,
    modelHz: FACTS.captureFps,
    visualScale: LAB.visualScale,
    modelMs: 0,
    camera: { present: true, node: "uvc-capture", hintOpen: false },
    capture: { running: true, seq: 0, fps: FACTS.captureFps },
    slot: { seq: 0, generation: 0, occupied: false },
    http: {
      clientSpeed: 1,
      lastSentSeq: 0,
      dropped: [],
      showDropped: false,
      sendDurationMs: sendDurationMs(1),
      busy: false,
      busyUntil: 0
    },
    motion: {
      sampleFps: FACTS.motionSampleFps,
      pixelDelta: FACTS.pixelDelta,
      changedRatioThreshold: FACTS.changedRatio,
      cooldownMs: FACTS.cooldownMs,
      score: 0,
      changedPixels: 0,
      totalPixels: FACTS.grayWidth * FACTS.grayHeight,
      gate: "idle",
      uiPhase: "idle",
      cooldownRemainingMs: 0,
      cooldownStartedMs: 0,
      baselineReady: false
    },
    jsonl: { lines: [], eventCountProcess: 0, seededDemo: false },
    proc: { pid: 2000, threads: 3, alive: true },
    systemd: {
      enabled: true,
      active: "active",
      nRestarts: 0,
      result: "success",
      execMainStatus: 0
    },
    status: {
      ok: true,
      degraded: false,
      health: "ok",
      camera_state: "active",
      event_log_state: "ok",
      event_count: 0,
      motion_state: false
    },
    eventLog: { writable: true, pendingEnospc: false },
    fault: { kind: "none", pendingRestartAtMs: null }
  };
}

function parseChapter(hash) {
  const name = (hash || "#frame").replace("#", "");
  if (name === "motion" || name === "fault" || name === "frame") {
    return name;
  }
  return "frame";
}

function stepHttpCapture(state, dt) {
  const next = { ...state, modelMs: state.modelMs + dt };
  if (!next.proc.alive || !next.capture.running || !next.camera.present) {
    return next;
  }
  const seq = next.capture.seq + 1;
  next.capture = { ...next.capture, seq };
  next.slot = { ...next.slot, seq, occupied: true };
  const http = { ...next.http };
  if (!http.busy) {
    const copied = seq;
    const result = skippedSeqs(copied, http.sendDurationMs);
    http.busy = true;
    http.busyUntil = next.modelMs + http.sendDurationMs;
    http.lastSentSeq = copied;
    http.dropped = [...http.dropped, ...result.skipped].slice(-48);
  } else if (next.modelMs >= http.busyUntil) {
    http.busy = false;
  }
  next.http = http;
  return next;
}

const store = createStore(initialState());
const reduced = () => window.matchMedia("(prefers-reduced-motion: reduce)").matches;
const motionBench = mountBenchMotion(store);

mountCrossSection(document.getElementById("cross-section"), store);
mountBenchFrame(store);
mountBenchFault(store);

function applyHash() {
  const chapter = parseChapter(location.hash);
  store.patch((s) => {
    let next = { ...s, chapter };
    if (chapter === "fault" && next.jsonl.lines.length === 0) {
      next = seedDemoLine(next, next.modelMs);
    }
    return next;
  });
  document.querySelectorAll(".chapter-nav a").forEach((a) => {
    a.setAttribute("aria-current", a.getAttribute("href") === `#${chapter}` ? "true" : "false");
  });
}

window.addEventListener("hashchange", applyHash);
applyHash();

const sections = ["frame", "motion", "fault"].map((id) => document.getElementById(id));
const observer = new IntersectionObserver(
  (entries) => {
    const visible = entries.filter((e) => e.isIntersecting).sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];
    if (!visible) {
      return;
    }
    const chapter = visible.target.id;
    if (store.get().chapter !== chapter) {
      history.replaceState(null, "", `#${chapter}`);
      applyHash();
    }
  },
  { rootMargin: "-30% 0px -50% 0px", threshold: [0.2, 0.4] }
);
sections.forEach((el) => observer.observe(el));

const clock = createClock({
  hz: FACTS.captureFps,
  maxStepsPerFrame: 4,
  isPlaying: () => store.get().playing,
  reducedMotion: reduced,
  onModelStep(dt) {
    store.patch((s) => {
      let next = stepHttpCapture(s, dt);
      next = motionBench.sampleIfDue(next, next.modelMs);
      return next;
    });
  },
  onFrame(now) {
    store.patch((s) => faultTick(s, now));
  }
});
clock.start();
window.__labReady = true;

if (reduced()) {
  document.getElementById("play-btn").addEventListener("click", () => {
    if (!store.get().playing) {
      clock.stepOnce(performance.now());
    }
  });
}
