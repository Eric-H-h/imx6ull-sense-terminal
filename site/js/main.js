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
import { mountArchitecture } from "./architecture-flow.js";

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

function parseLabTab(hash) {
  if (hash === "#lab-motion" || hash === "#motion") {
    return "motion";
  }
  if (hash === "#lab-fault" || hash === "#fault") {
    return "fault";
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
const tabs = {
  frame: document.getElementById("tab-frame"),
  motion: document.getElementById("tab-motion"),
  fault: document.getElementById("tab-fault")
};
const panels = {
  frame: document.getElementById("panel-frame"),
  motion: document.getElementById("panel-motion"),
  fault: document.getElementById("panel-fault")
};

const clockRef = { stepOnce: () => {} };

const navBar = document.querySelector(".site-nav");
const navToggle = document.getElementById("nav-toggle");
function setNavOpen(open) {
  navBar.classList.toggle("is-open", open);
  navToggle.setAttribute("aria-expanded", open ? "true" : "false");
}
navToggle.addEventListener("click", () => {
  setNavOpen(!navBar.classList.contains("is-open"));
});
document.querySelectorAll(".nav-links a").forEach((link) => {
  link.addEventListener("click", () => setNavOpen(false));
});

const tabOrder = ["frame", "motion", "fault"];

function setTab(name) {
  Object.entries(tabs).forEach(([key, btn]) => {
    const selected = key === name;
    btn.setAttribute("aria-selected", selected ? "true" : "false");
    btn.tabIndex = selected ? 0 : -1;
    panels[key].hidden = !selected;
  });
  store.patch((s) => {
    let next = { ...s, chapter: name };
    if (name === "fault" && next.jsonl.lines.length === 0) {
      next = seedDemoLine(next, next.modelMs);
    }
    return next;
  });
}

Object.entries(tabs).forEach(([name, btn]) => {
  btn.addEventListener("click", () => {
    const hash = name === "frame" ? "#lab" : `#lab-${name}`;
    history.replaceState(null, "", hash);
    setTab(name);
  });
  btn.addEventListener("keydown", (event) => {
    if (event.key !== "ArrowRight" && event.key !== "ArrowLeft" && event.key !== "Home" && event.key !== "End") {
      return;
    }
    event.preventDefault();
    const i = tabOrder.indexOf(name);
    let next = i;
    if (event.key === "ArrowRight") {
      next = (i + 1) % tabOrder.length;
    } else if (event.key === "ArrowLeft") {
      next = (i - 1 + tabOrder.length) % tabOrder.length;
    } else if (event.key === "Home") {
      next = 0;
    } else {
      next = tabOrder.length - 1;
    }
    tabs[tabOrder[next]].focus();
    tabs[tabOrder[next]].click();
  });
});

mountCrossSection(document.getElementById("cross-section"), store);
mountBenchFrame(store, {
  reduced: reduced(),
  stepOnce: (now) => clockRef.stepOnce(now)
});
mountBenchFault(store);
mountArchitecture(document.getElementById("architecture"));

function applyHash() {
  const hash = location.hash || "#overview";
  const navId = ["overview", "architecture", "decisions", "lab", "verification"].find((id) =>
    hash === `#${id}` || hash.startsWith(`#lab`)
  );
  document.querySelectorAll(".nav-links a").forEach((a) => {
    const href = a.getAttribute("href");
    const current = href === "#lab" ? hash === "#lab" || hash.startsWith("#lab-") : href === hash;
    a.setAttribute("aria-current", current ? "true" : "false");
  });
  if (hash.startsWith("#lab") || hash === "#motion" || hash === "#fault" || hash === "#frame") {
    setTab(parseLabTab(hash));
  }
}

window.addEventListener("hashchange", applyHash);
applyHash();

const navSections = ["overview", "architecture", "decisions", "lab", "verification"];
const navObserver = new IntersectionObserver(
  (entries) => {
    const vis = entries.filter((e) => e.isIntersecting).sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];
    if (!vis) {
      return;
    }
    const id = vis.target.id;
    document.querySelectorAll(".nav-links a").forEach((a) => {
      a.setAttribute("aria-current", a.getAttribute("href") === `#${id}` ? "true" : "false");
    });
  },
  { rootMargin: "-20% 0px -60% 0px", threshold: [0.2, 0.4] }
);
navSections.forEach((id) => {
  const el = document.getElementById(id);
  if (el) {
    navObserver.observe(el);
  }
});

const clock = createClock({
  hz: FACTS.captureFps,
  maxStepsPerFrame: 4,
  isPlaying: () => store.get().playing && !document.hidden,
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
clockRef.stepOnce = (now) => clock.stepOnce(now);
clock.start();
window.__labReady = true;
var labBoot = document.getElementById("lab-boot");
if (labBoot) {
  labBoot.hidden = true;
}
