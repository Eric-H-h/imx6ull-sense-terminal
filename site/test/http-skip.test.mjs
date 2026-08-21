import { test } from "node:test";
import assert from "node:assert/strict";
import { FRAME_DT_MS, skippedSeqs, sendDurationMs, CLIENT_SPEED_EPS } from "../js/http-skip.js";

test("sendDurationMs at speed 1 equals one frame", () => {
  assert.equal(sendDurationMs(1), FRAME_DT_MS);
});

test("sendDurationMs never divides by zero", () => {
  assert.equal(sendDurationMs(0), FRAME_DT_MS / CLIENT_SPEED_EPS);
});

test("skippedSeqs: one frame send copies the next seq", () => {
  assert.deepEqual(skippedSeqs(10, FRAME_DT_MS), { nextCopied: 11, skipped: [] });
});

test("skippedSeqs: two frame send skips one", () => {
  assert.deepEqual(skippedSeqs(10, 2 * FRAME_DT_MS), { nextCopied: 12, skipped: [11] });
});

test("skippedSeqs: twenty frame send skips 19", () => {
  const result = skippedSeqs(10, 20 * FRAME_DT_MS);
  assert.equal(result.skipped.length, 19);
  assert.equal(result.nextCopied, 30);
});
