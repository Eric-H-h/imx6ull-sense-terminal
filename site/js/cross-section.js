const HOT = {
  frame: ["camera", "capture", "slot", "http"],
  motion: ["slot", "motion", "jsonl"],
  fault: ["camera", "http", "motion", "jsonl"]
};

export function mountCrossSection(root, store) {
  const fields = {
    camera: root.querySelector("[data-field=camera]"),
    capture: root.querySelector("[data-field=capture]"),
    slot: root.querySelector("[data-field=slot]"),
    http: root.querySelector("[data-field=http]"),
    motion: root.querySelector("[data-field=motion]"),
    sys: root.querySelector("[data-field=sys]"),
    jsonl: root.querySelector("[data-field=jsonl]")
  };

  store.subscribe((state) => {
    const hot = new Set(HOT[state.chapter] || HOT.frame);
    if (state.chapter === "fault") {
      hot.add("camera");
    }
    root.querySelectorAll(".block").forEach((el) => {
      const name = el.getAttribute("data-block");
      el.dataset.hot = hot.has(name) ? "1" : "0";
    });
    const cam = state.camera.present
      ? state.proc.alive
        ? "uvc active"
        : "在位，进程已死"
      : "unavailable";
    fields.camera.textContent = cam;
    fields.capture.textContent = `${state.capture.fps} FPS${state.visualScale !== 1 ? " · 画面 ×0.4" : ""}`;
    fields.slot.textContent = `seq ${state.slot.seq}`;
    fields.http.textContent = `sent=${state.http.lastSentSeq} skip=${state.http.dropped.length}`;
    fields.motion.textContent = `${state.motion.sampleFps} FPS ${state.motion.uiPhase}`;
    fields.sys.textContent = `systemd ${state.systemd.active} NRestarts=${state.systemd.nRestarts}`;
    fields.jsonl.textContent = `jsonl ${state.jsonl.lines.length} 行 · event_count=${state.status.event_count}`;
  });
}
