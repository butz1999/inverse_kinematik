import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import vm from "node:vm";

const reportingSource = await readFile(new URL("../../web/controller-reporting.js", import.meta.url), "utf8");
const reportingContext = {};
vm.runInNewContext(reportingSource, { globalThis: reportingContext });
const {
  isCurrentControllerPollingGeneration,
  isReportableTargetPose,
  poseAfterRejectedMotion,
  shouldUpdateControllerMotionForms,
} = reportingContext.controllerReporting;

test("controller reporting respects the form update interval", () => {
  assert.equal(shouldUpdateControllerMotionForms({
    nowMs: 400,
    force: false,
    responseGeneration: 2,
    currentGeneration: 2,
    suppressUntilMs: 0,
    lastUpdateMs: 0,
    minimumIntervalMs: 500,
  }), false);
  assert.equal(shouldUpdateControllerMotionForms({
    nowMs: 500,
    force: false,
    responseGeneration: 2,
    currentGeneration: 2,
    suppressUntilMs: 0,
    lastUpdateMs: 0,
    minimumIntervalMs: 500,
  }), true);
});

test("controller reporting does not overwrite manual input during suppression", () => {
  assert.equal(shouldUpdateControllerMotionForms({
    nowMs: 1000,
    force: true,
    responseGeneration: 2,
    currentGeneration: 2,
    suppressUntilMs: 1001,
    lastUpdateMs: 0,
    minimumIntervalMs: 500,
  }), false);
});

test("controller reporting discards stale polling responses after a restart", () => {
  assert.equal(isCurrentControllerPollingGeneration(3, 4), false);
  assert.equal(isCurrentControllerPollingGeneration(4, 4), true);
});

test("controller reporting rejects invalid target poses", () => {
  const validPose = { x_mm: 1, y_mm: 2, z_mm: 3, p_deg: -90, r_deg: 0, g_pct: 50 };
  assert.equal(isReportableTargetPose(validPose), true);
  assert.equal(isReportableTargetPose({ ...validPose, x_mm: Number.NaN }), false);
  assert.equal(isReportableTargetPose({ ...validPose, g_pct: 101 }), false);
  assert.equal(isReportableTargetPose({ ...validPose, r_deg: null }), false);
});

test("rejected pose motion restores the last confirmed pose", () => {
  const confirmedPose = { x_mm: 1, y_mm: 2, z_mm: 3, p_deg: -90, r_deg: 0, g_pct: 50 };
  assert.equal(JSON.stringify(poseAfterRejectedMotion(confirmedPose)), JSON.stringify(confirmedPose));
  assert.equal(poseAfterRejectedMotion({ ...confirmedPose, g_pct: -1 }), null);
});
