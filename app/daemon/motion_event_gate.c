#include "motion_event_gate.h"

#include <stdint.h>
#include <string.h>

static int phase_is_valid(MotionEventPhase phase)
{
    return (phase == MOTION_EVENT_IDLE) ||
           (phase == MOTION_EVENT_COOLDOWN);
}

static void emit_event(MotionEventGate *gate, uint64_t now_ms)
{
    gate->phase = MOTION_EVENT_COOLDOWN;
    gate->cooldown_started_ms = now_ms;
    if (gate->event_count != UINT64_MAX) {
        ++gate->event_count;
    }
}

static void fill_decision(const MotionEventGate *gate,
                          int event_emitted,
                          uint64_t cooldown_remaining_ms,
                          MotionEventDecision *decision)
{
    decision->event_emitted = event_emitted;
    decision->phase = gate->phase;
    decision->event_count = gate->event_count;
    decision->cooldown_remaining_ms = cooldown_remaining_ms;
}

MotionEventGateStatus motion_event_gate_update(MotionEventGate *gate,
                                               int motion_detected,
                                               uint64_t now_ms,
                                               uint64_t cooldown_ms,
                                               MotionEventDecision *decision)
{
    uint64_t elapsed_ms;
    uint64_t remaining_ms = 0U;
    int event_emitted = 0;

    if (decision != NULL) {
        memset(decision, 0, sizeof(*decision));
    }
    if ((gate == NULL) || (decision == NULL) ||
        ((motion_detected != 0) && (motion_detected != 1)) ||
        (cooldown_ms == 0U)) {
        return MOTION_EVENT_GATE_INVALID_ARGUMENT;
    }
    if (!phase_is_valid(gate->phase)) {
        return MOTION_EVENT_GATE_INVALID_STATE;
    }

    if (gate->phase == MOTION_EVENT_IDLE) {
        if (motion_detected != 0) {
            emit_event(gate, now_ms);
            event_emitted = 1;
            remaining_ms = cooldown_ms;
        }
        fill_decision(gate, event_emitted, remaining_ms, decision);
        return MOTION_EVENT_GATE_OK;
    }

    if (now_ms < gate->cooldown_started_ms) {
        fill_decision(gate, 0, cooldown_ms, decision);
        return MOTION_EVENT_GATE_OK;
    }

    elapsed_ms = now_ms - gate->cooldown_started_ms;
    if (elapsed_ms < cooldown_ms) {
        remaining_ms = cooldown_ms - elapsed_ms;
        fill_decision(gate, 0, remaining_ms, decision);
        return MOTION_EVENT_GATE_OK;
    }

    gate->phase = MOTION_EVENT_IDLE;
    if (motion_detected != 0) {
        emit_event(gate, now_ms);
        event_emitted = 1;
        remaining_ms = cooldown_ms;
    }

    fill_decision(gate, event_emitted, remaining_ms, decision);
    return MOTION_EVENT_GATE_OK;
}

void motion_event_gate_reset(MotionEventGate *gate)
{
    if (gate == NULL) {
        return;
    }

    memset(gate, 0, sizeof(*gate));
}

const char *motion_event_phase_string(MotionEventPhase phase)
{
    switch (phase) {
    case MOTION_EVENT_IDLE:
        return "idle";
    case MOTION_EVENT_COOLDOWN:
        return "cooldown";
    default:
        return "unknown motion event phase";
    }
}

const char *motion_event_gate_status_string(MotionEventGateStatus status)
{
    switch (status) {
    case MOTION_EVENT_GATE_OK:
        return "ok";
    case MOTION_EVENT_GATE_INVALID_ARGUMENT:
        return "invalid argument";
    case MOTION_EVENT_GATE_INVALID_STATE:
        return "invalid state";
    default:
        return "unknown motion event gate status";
    }
}