import { FACTS } from "./evidence-facts.js";
import { attemptMotionEmit, rasterBlobFrame } from "./motion-math.js";
import { LAB } from "./lab-model.js";

function nextPid(pid) {
  return pid + 17;
}

function healthyStatus() {
  return {
    ok: true,
    degraded: false,
    health: "ok",
    camera_state: "active",
    event_log_state: "ok",
    event_count: 0,
    motion_state: false
  };
}

function applyDegraded(status, patch) {
  const next = { ...status, ...patch };
  const degraded =
    next.camera_state !== "active" || next.event_log_state === "unavailable";
  next.degraded = degraded;
  next.ok = !degraded;
  next.health = degraded ? "degraded" : "ok";
  return next;
}

export function seedDemoLine(state, nowMs) {
  if (state.jsonl.lines.length > 0) {
    return state;
  }
  const line =
    '{"ts_ms":0,"type":"motion","sequence":0,"score":0.070000,"threshold":0.050000,"changed_pixels":1344,"total_pixels":19200,"cooldown_ms":1500}';
  return {
    ...state,
    jsonl: {
      ...state.jsonl,
      lines: [`${line} /* 演示种子，不是板上 reboot/kill 产生的 */`],
      seededDemo: true
    }
  };
}

export function applyAction(state, action, wallNowMs) {
  if (action === "unplug") {
    return {
      ...state,
      camera: { ...state.camera, present: false },
      motion: { ...state.motion, baselineReady: false, score: 0, uiPhase: "idle" },
      status: applyDegraded(state.status, { camera_state: "unavailable" }),
      fault: { ...state.fault, kind: "unplug" }
    };
  }
  if (action === "plug") {
    return {
      ...state,
      camera: { ...state.camera, present: true },
      motion: { ...state.motion, baselineReady: false },
      status: applyDegraded(state.status, { camera_state: "active" }),
      fault: { ...state.fault, kind: "none" }
    };
  }
  if (action === "sigterm") {
    return {
      ...state,
      proc: { ...state.proc, alive: false, threads: 0 },
      capture: { ...state.capture, running: false },
      systemd: {
        ...state.systemd,
        active: "inactive",
        result: "success",
        execMainStatus: 0
      },
      fault: { ...state.fault, kind: "sigterm", pendingRestartAtMs: null }
    };
  }
  if (action === "kill9") {
    return {
      ...state,
      proc: { ...state.proc, alive: false, threads: 0 },
      capture: { ...state.capture, running: false },
      systemd: {
        ...state.systemd,
        active: "activating",
        result: "signal"
      },
      fault: {
        kind: "kill9",
        pendingRestartAtMs: wallNowMs + FACTS.restartSec * 1000
      }
    };
  }
  if (action === "badconfig") {
    return {
      ...state,
      proc: { ...state.proc, alive: false, threads: 0 },
      capture: { ...state.capture, running: false },
      systemd: {
        enabled: true,
        active: "failed",
        nRestarts: 0,
        result: "exit-code",
        execMainStatus: FACTS.configExit
      },
      status: applyDegraded(state.status, { camera_state: "unavailable" }),
      fault: { kind: "badconfig", pendingRestartAtMs: null }
    };
  }
  if (action === "enospc") {
    return {
      ...state,
      eventLog: { writable: true, pendingEnospc: true },
      fault: { ...state.fault, kind: "enospc" }
    };
  }
  if (action === "tryAppend") {
    return tryAppend(state, wallNowMs);
  }
  if (action === "reboot") {
    const pid = nextPid(state.proc.pid);
    return {
      ...state,
      proc: { pid, threads: 3, alive: true },
      capture: { running: true, seq: 0, fps: FACTS.captureFps },
      slot: { seq: 0, generation: state.slot.generation + 1, occupied: false },
      camera: { ...state.camera, present: true },
      motion: {
        ...state.motion,
        score: 0,
        gate: "idle",
        uiPhase: "idle",
        cooldownRemainingMs: 0,
        baselineReady: false
      },
      jsonl: { ...state.jsonl, eventCountProcess: 0 },
      systemd: {
        enabled: true,
        active: "active",
        nRestarts: 0,
        result: "success",
        execMainStatus: 0
      },
      status: { ...healthyStatus(), motion_state: false },
      eventLog: { writable: true, pendingEnospc: false },
      fault: { kind: "none", pendingRestartAtMs: null }
    };
  }
  if (action === "reset") {
    return {
      ...state,
      camera: { present: true, node: "uvc-capture", hintOpen: false },
      capture: { running: true, seq: state.capture.seq, fps: FACTS.captureFps },
      proc: { ...state.proc, alive: true, threads: 3 },
      motion: {
        ...state.motion,
        gate: "idle",
        uiPhase: "idle",
        cooldownRemainingMs: 0,
        baselineReady: false
      },
      systemd: {
        enabled: true,
        active: "active",
        nRestarts: 0,
        result: "success",
        execMainStatus: 0
      },
      eventLog: { writable: true, pendingEnospc: false },
      status: { ...healthyStatus(), event_count: state.jsonl.eventCountProcess },
      fault: { kind: "none", pendingRestartAtMs: null }
    };
  }
  return state;
}

export function tryAppend(state, wallNowMs) {
  if (!state.eventLog.pendingEnospc || !state.proc.alive) {
    return state;
  }
  const w = FACTS.grayWidth;
  const h = FACTS.grayHeight;
  const cy = h / 2;
  const prev = rasterBlobFrame(w, h, 80, cy, LAB.blob);
  const curr = rasterBlobFrame(w, h, 80 + LAB.blob.waveDxPerSample, cy, LAB.blob);
  const gate = {
    phase: "idle",
    cooldownStartedMs: 0,
    eventCount: state.jsonl.eventCountProcess
  };
  if (state.motion.gate === "cooldown") {
    gate.phase = "cooldown";
    gate.cooldownStartedMs = wallNowMs - state.motion.cooldownMs;
  }
  const emitted = attemptMotionEmit({
    prev,
    curr,
    width: w,
    height: h,
    pixelDelta: state.motion.pixelDelta,
    ratioThreshold: state.motion.changedRatioThreshold,
    gate,
    nowMs: wallNowMs,
    cooldownMs: state.motion.cooldownMs || FACTS.cooldownMs,
    sequence: state.capture.seq,
    writable: false
  });
  const eventCount = emitted.decision.eventCount;
  return {
    ...state,
    jsonl: { ...state.jsonl, eventCountProcess: eventCount },
    eventLog: { writable: false, pendingEnospc: true },
    status: applyDegraded(
      {
        ...state.status,
        event_count: eventCount,
        event_log_state: "unavailable",
        camera_state: state.camera.present ? "active" : "unavailable",
        motion_state: emitted.score.motionDetected
      },
      {}
    ),
    motion: {
      ...state.motion,
      gate: emitted.decision.phase,
      uiPhase: emitted.uiPhase,
      score: emitted.score.changedRatio,
      cooldownRemainingMs: emitted.decision.cooldownRemainingMs
    }
  };
}

export function tick(state, wallNowMs) {
  const due =
    state.fault.pendingRestartAtMs != null &&
    wallNowMs >= state.fault.pendingRestartAtMs;
  if (!due) {
    return state;
  }
  const pid = nextPid(state.proc.pid);
  return {
    ...state,
    proc: { pid, threads: 3, alive: true },
    capture: { running: true, seq: 0, fps: FACTS.captureFps },
    slot: { seq: 0, generation: state.slot.generation + 1, occupied: false },
    jsonl: { ...state.jsonl, eventCountProcess: 0 },
    systemd: {
      enabled: true,
      active: "active",
      nRestarts: state.systemd.nRestarts + 1,
      result: "success",
      execMainStatus: 0
    },
    status: {
      ...healthyStatus(),
      camera_state: state.camera.present ? "active" : "unavailable",
      event_log_state: state.eventLog.writable ? "ok" : "unavailable"
    },
    fault: { kind: "none", pendingRestartAtMs: null },
    motion: {
      ...state.motion,
      baselineReady: false,
      gate: "idle",
      uiPhase: "idle",
      cooldownRemainingMs: 0,
      score: 0
    }
  };
}

export { healthyStatus, nextPid };
