import { test } from "node:test";
import assert from "node:assert/strict";
import { FACTS } from "../js/evidence-facts.js";

test("board facts match v0.1-mvp test-report defaults", () => {
  assert.equal(FACTS.pixelDelta, 25);
  assert.equal(FACTS.changedRatio, 0.05);
  assert.equal(FACTS.cooldownMs, 1500);
  assert.equal(FACTS.motionSampleFps, 3.0);
  assert.equal(FACTS.grayWidth, 160);
  assert.equal(FACTS.grayHeight, 120);
  assert.equal(FACTS.grayWidth * FACTS.grayHeight, 19200);
  assert.equal(FACTS.restartSec, 3);
  assert.equal(FACTS.configExit, 78);
  assert.equal(FACTS.captureFps, 30.0);
  assert.equal(FACTS.rssKb, 2968);
});
