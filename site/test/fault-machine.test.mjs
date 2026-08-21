import { test } from "node:test";
import assert from "node:assert/strict";
import { applyAction, tick, seedDemoLine } from "../js/fault-machine.js";
import { FACTS } from "../js/evidence-facts.js";

function healthy() {
  return {
    camera: { present: true, node: "uvc-capture", hintOpen: false },
    capture: { running: true, seq: 10, fps: 30 },
    slot: { seq: 10, generation: 1, occupied: true },
    motion: {
      sampleFps: 3,
      pixelDelta: 25,
      changedRatioThreshold: 0.05,
      cooldownMs: 1500,
      score: 0,
      gate: "idle",
      uiPhase: "idle",
      cooldownRemainingMs: 0,
      baselineReady: true
    },
    jsonl: { lines: ["seed"], eventCountProcess: 2, seededDemo: true },
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
      event_count: 2,
      motion_state: false
    },
    eventLog: { writable: true, pendingEnospc: false },
    fault: { kind: "none", pendingRestartAtMs: null }
  };
}

test("unplug keeps pid and degrades camera", () => {
  const s = applyAction(healthy(), "unplug", 0);
  assert.equal(s.proc.pid, 2000);
  assert.equal(s.proc.alive, true);
  assert.equal(s.status.health, "degraded");
  assert.equal(s.status.camera_state, "unavailable");
  assert.equal(s.jsonl.lines.length, 1);
  assert.equal(s.systemd.nRestarts, 0);
});

test("plug restores camera without new pid", () => {
  const s = applyAction(applyAction(healthy(), "unplug", 0), "plug", 1);
  assert.equal(s.proc.pid, 2000);
  assert.equal(s.status.camera_state, "active");
  assert.equal(s.status.health, "ok");
});

test("SIGTERM does not restart", () => {
  const s = applyAction(healthy(), "sigterm", 0);
  assert.equal(s.proc.alive, false);
  assert.equal(s.systemd.active, "inactive");
  assert.equal(s.systemd.result, "success");
  assert.equal(s.systemd.execMainStatus, 0);
  assert.equal(s.systemd.nRestarts, 0);
  const later = tick(s, 10000);
  assert.equal(later.proc.alive, false);
});

test("kill-9 uses wall clock RestartSec even if model paused", () => {
  const t0 = 1000;
  const s = applyAction(healthy(), "kill9", t0);
  assert.equal(s.proc.alive, false);
  assert.equal(s.camera.present, true);
  const early = tick(s, t0 + FACTS.restartSec * 1000 - 1);
  assert.equal(early.proc.alive, false);
  const late = tick(s, t0 + FACTS.restartSec * 1000);
  assert.equal(late.proc.alive, true);
  assert.notEqual(late.proc.pid, 2000);
  assert.equal(late.jsonl.eventCountProcess, 0);
  assert.equal(late.jsonl.lines.length, 1);
  assert.equal(late.systemd.nRestarts, 1);
  assert.equal(late.status.event_count, 0);
  assert.equal(typeof late.status.motion_state, "boolean");
});

test("bad config start exits 78 without restart storm", () => {
  const s = applyAction(healthy(), "badconfig", 0);
  assert.equal(s.proc.alive, false);
  assert.equal(s.systemd.active, "failed");
  assert.equal(s.systemd.execMainStatus, 78);
  assert.equal(s.systemd.nRestarts, 0);
  assert.equal(tick(s, 99999).proc.alive, false);
});

test("enospc trap then tryAppend increments event_count not file", () => {
  const armed = applyAction(healthy(), "enospc", 0);
  assert.equal(armed.status.event_log_state, "ok");
  assert.equal(armed.eventLog.pendingEnospc, true);
  const noop = applyAction(healthy(), "tryAppend", 0);
  assert.equal(noop.jsonl.eventCountProcess, 2);
  assert.equal(noop.jsonl.lines.length, 1);
  const failed = applyAction(armed, "tryAppend", 5000);
  assert.equal(failed.jsonl.eventCountProcess, 3);
  assert.equal(failed.jsonl.lines.length, 1);
  assert.equal(failed.status.event_log_state, "unavailable");
  assert.equal(failed.status.health, "degraded");
  assert.equal(failed.status.camera_state, "active");
});

test("reboot keeps jsonl and zeros process event_count", () => {
  const s = applyAction(healthy(), "reboot", 0);
  assert.notEqual(s.proc.pid, 2000);
  assert.equal(s.jsonl.lines.length, 1);
  assert.equal(s.jsonl.eventCountProcess, 0);
  assert.equal(s.systemd.nRestarts, 0);
  assert.equal(s.proc.alive, true);
});

test("seed demo line does not bump event_count", () => {
  const empty = { ...healthy(), jsonl: { lines: [], eventCountProcess: 0, seededDemo: false } };
  const s = seedDemoLine(empty, 0);
  assert.equal(s.jsonl.lines.length, 1);
  assert.equal(s.jsonl.eventCountProcess, 0);
  assert.equal(s.jsonl.seededDemo, true);
});
