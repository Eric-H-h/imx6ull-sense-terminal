import { test } from "node:test";
import assert from "node:assert/strict";
import {
  scoreGrayFrames,
  gateUpdate,
  formatJsonlLine,
  rasterBlobFrame,
  attemptMotionEmit
} from "../js/motion-math.js";
import { LAB } from "../js/lab-model.js";
import { FACTS } from "../js/evidence-facts.js";

test("first frame is baseline only", () => {
  const curr = new Uint8Array(8).fill(10);
  const r = scoreGrayFrames(null, curr, 4, 2, 25, 0.05);
  assert.equal(r.baselineCreated, true);
  assert.equal(r.changedPixels, 0);
  assert.equal(r.motionDetected, false);
});

test("identical frames score zero", () => {
  const a = new Uint8Array(8).fill(10);
  const r = scoreGrayFrames(a, a.slice(), 4, 2, 25, 0.05);
  assert.equal(r.changedRatio, 0);
  assert.equal(r.motionDetected, false);
});

test("pixel delta uses inclusive >=", () => {
  const prev = new Uint8Array([0, 0, 0, 0]);
  const curr = new Uint8Array([25, 24, 0, 0]);
  const r = scoreGrayFrames(prev, curr, 2, 2, 25, 0.05);
  assert.equal(r.changedPixels, 1);
});

test("half the pixels changed yields ratio 0.5", () => {
  const prev = new Uint8Array(8).fill(0);
  const curr = new Uint8Array([200, 200, 200, 200, 0, 0, 0, 0]);
  const r = scoreGrayFrames(prev, curr, 4, 2, 25, 0.05);
  assert.equal(r.changedRatio, 0.5);
  assert.equal(r.motionDetected, true);
});

test("IDLE first detection emits then cooldown", () => {
  const idle = { phase: "idle", cooldownStartedMs: 0, eventCount: 0 };
  const d = gateUpdate(idle, 1, 1000, 1500);
  assert.equal(d.eventEmitted, 1);
  assert.equal(d.phase, "cooldown");
  assert.equal(d.eventCount, 1);
  const hold = gateUpdate(d.gate, 1, 2000, 1500);
  assert.equal(hold.eventEmitted, 0);
});

test("cooldown boundary may emit immediately", () => {
  const g = { phase: "cooldown", cooldownStartedMs: 1000, eventCount: 1 };
  const d = gateUpdate(g, 1, 2500, 1500);
  assert.equal(d.eventEmitted, 1);
  assert.equal(d.eventCount, 2);
});

test("counterfactual cooldownMs=0 emits every detection", () => {
  const g = { phase: "idle", cooldownStartedMs: 0, eventCount: 0 };
  const a = gateUpdate(g, 1, 1, 0);
  const b = gateUpdate(a.gate, 1, 2, 0);
  assert.equal(a.eventEmitted, 1);
  assert.equal(b.eventEmitted, 1);
  assert.equal(b.eventCount, 2);
  assert.equal(b.counterfactual, true);
});

test("formatJsonlLine matches C golden string cooldown 3000", () => {
  const line = formatJsonlLine({
    ts_ms: 1784973600123,
    sequence: 3821,
    score: 0.125,
    threshold: 0.05,
    changed_pixels: 2400,
    total_pixels: 19200,
    cooldown_ms: 3000
  });
  assert.equal(
    line,
    '{"ts_ms":1784973600123,"type":"motion","sequence":3821,"score":0.125000,"threshold":0.050000,"changed_pixels":2400,"total_pixels":19200,"cooldown_ms":3000}'
  );
});

test("formatJsonlLine board default cooldown 1500", () => {
  const line = formatJsonlLine({
    ts_ms: 1,
    sequence: 1,
    score: 0.07,
    threshold: FACTS.changedRatio,
    changed_pixels: 1344,
    total_pixels: 19200,
    cooldown_ms: FACTS.cooldownMs
  });
  assert.match(line, /"cooldown_ms":1500/);
  assert.match(line, /"threshold":0.050000/);
});

test("LAB.blob wave XOR sits between 5% and 20%", () => {
  const w = FACTS.grayWidth;
  const h = FACTS.grayHeight;
  const cy = h / 2;
  const cx = 80;
  const idle = rasterBlobFrame(w, h, cx, cy);
  const wave = rasterBlobFrame(w, h, cx + LAB.blob.waveDxPerSample, cy);
  const r = scoreGrayFrames(idle, wave, w, h, FACTS.pixelDelta, FACTS.changedRatio);
  assert.ok(r.changedRatio > 0.05, `ratio ${r.changedRatio} <= 0.05`);
  assert.ok(r.changedRatio < 0.20, `ratio ${r.changedRatio} >= 0.20`);
  const high = scoreGrayFrames(idle, wave, w, h, FACTS.pixelDelta, 0.20);
  assert.equal(high.motionDetected, false);
  assert.equal(r.motionDetected, true);
});

test("attemptMotionEmit write failure still counts gate emit", () => {
  const w = 160;
  const h = 120;
  const prev = rasterBlobFrame(w, h, 80, 60);
  const curr = rasterBlobFrame(w, h, 80 + LAB.blob.waveDxPerSample, 60);
  const r = attemptMotionEmit({
    prev,
    curr,
    width: w,
    height: h,
    pixelDelta: 25,
    ratioThreshold: 0.05,
    gate: { phase: "idle", cooldownStartedMs: 0, eventCount: 0 },
    nowMs: 5000,
    cooldownMs: 1500,
    sequence: 9,
    writable: false
  });
  assert.equal(r.decision.eventEmitted, 1);
  assert.equal(r.decision.eventCount, 1);
  assert.equal(r.wrote, false);
  assert.equal(r.writeFailed, true);
});
