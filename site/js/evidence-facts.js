const BLOB = "https://github.com/Eric-H-h/imx6ull-sense-terminal/blob/v0.1-mvp/";

export const FACTS = {
  captureFps: 30.0,
  width: 640,
  height: 480,
  motionSampleFps: 3.0,
  grayWidth: 160,
  grayHeight: 120,
  pixelDelta: 25,
  changedRatio: 0.05,
  cooldownMs: 1500,
  restartSec: 3,
  configExit: 78,
  jsonlPath: "/var/lib/imx6ull-sense/events.jsonl",
  sources: {
    testReport: `${BLOB}docs/verification/test-report.md`,
    m2: `${BLOB}docs/verification/evidence/M2_mjpeg_stream.md`,
    m3: `${BLOB}docs/verification/evidence/M3_motion_event.md`,
    m4: `${BLOB}docs/verification/evidence/M4_fault_injection.md`,
    httpApi: `${BLOB}docs/reference/http-api.md`,
    configuration: `${BLOB}docs/reference/configuration.md`,
    pxp: `${BLOB}docs/bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md`,
    mjpegExplain: `${BLOB}docs/explanation/mjpeg-over-http.md`
  }
};
