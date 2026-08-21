export const FRAME_DT_MS = 1000 / 30;
export const CLIENT_SPEED_EPS = 0.05;

export function sendDurationMs(clientSpeed) {
  return FRAME_DT_MS / Math.max(clientSpeed, CLIENT_SPEED_EPS);
}

export function skippedSeqs(startSeq, sendDurationMs, publishDtMs = FRAME_DT_MS) {
  const framesDuringSend = Math.floor(sendDurationMs / publishDtMs);
  const latest = startSeq + Math.max(1, framesDuringSend);
  const skipped = [];
  for (let s = startSeq + 1; s < latest; s += 1) {
    skipped.push(s);
  }
  return { nextCopied: latest, skipped };
}
