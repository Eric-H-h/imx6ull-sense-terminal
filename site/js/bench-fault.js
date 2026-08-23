import { applyAction } from "./fault-machine.js";

const BUTTONS = [
  ["unplug", "拔出摄像头"],
  ["plug", "插回"],
  ["sigterm", "SIGTERM"],
  ["kill9", "kill -9"],
  ["badconfig", "以非法配置启动"],
  ["enospc", "日志 ENOSPC"],
  ["tryAppend", "尝试追加一次"],
  ["reboot", "reboot"],
  ["reset", "恢复现场"]
];

export function mountBenchFault(store) {
  const bar = document.getElementById("fault-buttons");
  const procCol = document.getElementById("proc-col");
  const statusCol = document.getElementById("status-col");
  const jsonlCol = document.getElementById("jsonl-col");
  const systemdBar = document.getElementById("systemd-bar");
  const note = document.getElementById("fault-empty-note");
  bar.replaceChildren();
  BUTTONS.forEach(([action, label]) => {
    const btn = document.createElement("button");
    btn.type = "button";
    btn.dataset.action = action;
    btn.textContent = label;
    btn.addEventListener("click", () => {
      store.patch((s) => applyAction(s, action, performance.now()));
    });
    bar.append(btn);
  });

  store.subscribe((state) => {
    bar.querySelectorAll("button").forEach((btn) => {
      if (btn.dataset.action === "tryAppend") {
        btn.disabled = !(state.eventLog.pendingEnospc && state.proc.alive);
      }
      if (btn.dataset.action === "plug") {
        btn.disabled = state.camera.present;
      }
      if (btn.dataset.action === "unplug") {
        btn.disabled = !state.camera.present || !state.proc.alive;
      }
    });
    note.textContent = state.jsonl.seededDemo
      ? "下面有一行演示种子（不计 event_count）。本进程计数只在 gate emit 时增加，含写失败。"
      : "还没有 JSONL。可先到运动章写一行，再看 kill -9 是否保留文件。";
    procCol.textContent = state.proc.alive
      ? `PID ${state.proc.pid}\nthreads ${state.proc.threads}\nalive yes`
      : `PID —\nthreads 0\nalive no`;
    const subset = {
      ok: state.status.ok,
      degraded: state.status.degraded,
      health: state.status.health,
      camera_state: state.status.camera_state,
      event_log_state: state.status.event_log_state,
      event_count: state.status.event_count,
      motion_state: state.status.motion_state
    };
    statusCol.textContent = state.proc.alive ? JSON.stringify(subset, null, 2) : "无 HTTP";
    jsonlCol.textContent = `${state.jsonl.lines.length} 行\n${state.jsonl.lines.at(-1) || ""}`;
    systemdBar.textContent = `active=${state.systemd.active} NRestarts=${state.systemd.nRestarts} Result=${state.systemd.result} ExecMainStatus=${state.systemd.execMainStatus}`;
  });
}
