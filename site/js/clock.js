export function createClock({ hz, maxStepsPerFrame, onModelStep, onFrame, isPlaying, reducedMotion }) {
  const dt = 1000 / hz;
  let acc = 0;
  let last = 0;
  let raf = 0;
  let running = false;

  function frame(now) {
    if (!running) {
      return;
    }
    raf = requestAnimationFrame(frame);
    if (!isPlaying()) {
      last = now;
      onFrame(now);
      return;
    }
    if (last === 0) {
      last = now;
    }
    acc += now - last;
    last = now;
    let steps = 0;
    while (acc >= dt && steps < maxStepsPerFrame) {
      onModelStep(dt, now);
      acc -= dt;
      steps += 1;
    }
    if (steps === maxStepsPerFrame) {
      acc = 0;
    }
    onFrame(now);
  }

  return {
    start() {
      if (running) {
        return;
      }
      running = true;
      last = 0;
      acc = 0;
      raf = requestAnimationFrame(frame);
    },
    stop() {
      running = false;
      cancelAnimationFrame(raf);
    },
    stepOnce(now) {
      onModelStep(dt, now);
      onFrame(now);
    }
  };
}
