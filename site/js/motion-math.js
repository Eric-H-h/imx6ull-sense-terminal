import { FACTS } from "./evidence-facts.js";
import { LAB } from "./lab-model.js";

export function scoreGrayFrames(prev, curr, width, height, pixelDelta, ratioThreshold) {
  const total = width * height;
  if (prev == null) {
    return {
      baselineCreated: true,
      changedPixels: 0,
      totalPixels: total,
      changedRatio: 0,
      motionDetected: false
    };
  }
  let changed = 0;
  for (let i = 0; i < total; i += 1) {
    const delta = curr[i] > prev[i] ? curr[i] - prev[i] : prev[i] - curr[i];
    if (delta >= pixelDelta) {
      changed += 1;
    }
  }
  const changedRatio = changed / total;
  return {
    baselineCreated: false,
    changedPixels: changed,
    totalPixels: total,
    changedRatio,
    motionDetected: changedRatio >= ratioThreshold
  };
}

export function gateUpdate(gate, motionDetected, nowMs, cooldownMs) {
  const next = {
    phase: gate.phase,
    cooldownStartedMs: gate.cooldownStartedMs,
    eventCount: gate.eventCount
  };
  let eventEmitted = 0;
  let remaining = 0;

  if (cooldownMs === 0) {
    if (motionDetected) {
      next.eventCount += 1;
      eventEmitted = 1;
    }
    return {
      gate: next,
      eventEmitted,
      phase: "idle",
      eventCount: next.eventCount,
      cooldownRemainingMs: 0,
      counterfactual: true
    };
  }

  if (next.phase === "idle") {
    if (motionDetected) {
      next.phase = "cooldown";
      next.cooldownStartedMs = nowMs;
      next.eventCount += 1;
      eventEmitted = 1;
      remaining = cooldownMs;
    }
    return {
      gate: next,
      eventEmitted,
      phase: next.phase,
      eventCount: next.eventCount,
      cooldownRemainingMs: remaining,
      counterfactual: false
    };
  }

  if (nowMs < next.cooldownStartedMs) {
    return {
      gate: next,
      eventEmitted: 0,
      phase: next.phase,
      eventCount: next.eventCount,
      cooldownRemainingMs: cooldownMs,
      counterfactual: false
    };
  }

  const elapsed = nowMs - next.cooldownStartedMs;
  if (elapsed < cooldownMs) {
    return {
      gate: next,
      eventEmitted: 0,
      phase: next.phase,
      eventCount: next.eventCount,
      cooldownRemainingMs: cooldownMs - elapsed,
      counterfactual: false
    };
  }

  next.phase = "idle";
  if (motionDetected) {
    next.phase = "cooldown";
    next.cooldownStartedMs = nowMs;
    next.eventCount += 1;
    eventEmitted = 1;
    remaining = cooldownMs;
  }
  return {
    gate: next,
    eventEmitted,
    phase: next.phase,
    eventCount: next.eventCount,
    cooldownRemainingMs: remaining,
    counterfactual: false
  };
}

export function mapUiPhase(gatePhase, motionDetected, eventEmitted) {
  if (eventEmitted === 1 && motionDetected) {
    return "motion";
  }
  if (gatePhase === "cooldown") {
    return "cooldown";
  }
  return "idle";
}

export function formatJsonlLine(record) {
  const score = Number(record.score).toFixed(6);
  const threshold = Number(record.threshold).toFixed(6);
  return `{"ts_ms":${record.ts_ms},"type":"motion","sequence":${record.sequence},"score":${score},"threshold":${threshold},"changed_pixels":${record.changed_pixels},"total_pixels":${record.total_pixels},"cooldown_ms":${record.cooldown_ms}}`;
}

export function rasterBlobFrame(width, height, cx, cy, blob = LAB.blob) {
  const pixels = new Uint8Array(width * height);
  pixels.fill(blob.bgGray);
  const rx = blob.rx;
  const ry = blob.ry;
  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const nx = (x - cx) / rx;
      const ny = (y - cy) / ry;
      if (nx * nx + ny * ny <= 1) {
        pixels[y * width + x] = blob.blobGray;
      }
    }
  }
  return pixels;
}

export function attemptMotionEmit(input) {
  const {
    prev,
    curr,
    width,
    height,
    pixelDelta,
    ratioThreshold,
    gate,
    nowMs,
    cooldownMs,
    sequence,
    writable
  } = input;
  const score = scoreGrayFrames(prev, curr, width, height, pixelDelta, ratioThreshold);
  const decision = gateUpdate(gate, score.motionDetected ? 1 : 0, nowMs, cooldownMs);
  const uiPhase = mapUiPhase(decision.phase, score.motionDetected, decision.eventEmitted);
  const result = {
    score,
    decision,
    uiPhase,
    line: null,
    wrote: false,
    writeFailed: false
  };
  if (decision.eventEmitted !== 1) {
    return result;
  }
  const record = {
    ts_ms: Math.floor(nowMs),
    sequence,
    score: score.changedRatio,
    threshold: ratioThreshold,
    changed_pixels: score.changedPixels,
    total_pixels: score.totalPixels,
    cooldown_ms: cooldownMs
  };
  const line = formatJsonlLine(record);
  if (!writable) {
    result.writeFailed = true;
    result.line = cooldownMs === 0 ? `${line} /* counterfactual — C would reject this record */` : line;
    return result;
  }
  result.wrote = true;
  result.line = cooldownMs === 0 ? `${line} /* counterfactual — C would reject this record */` : line;
  return result;
}

export function defaultGraySize() {
  return { width: FACTS.grayWidth, height: FACTS.grayHeight };
}
