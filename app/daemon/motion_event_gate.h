#ifndef MOTION_EVENT_GATE_H
#define MOTION_EVENT_GATE_H

#include <stdint.h>

typedef enum {
    MOTION_EVENT_IDLE = 0,
    MOTION_EVENT_COOLDOWN = 1
} MotionEventPhase;

typedef enum {
    MOTION_EVENT_GATE_OK = 0,
    MOTION_EVENT_GATE_INVALID_ARGUMENT = -1,
    MOTION_EVENT_GATE_INVALID_STATE = -2
} MotionEventGateStatus;

typedef struct {
    MotionEventPhase phase;
    uint64_t cooldown_started_ms;
    uint64_t event_count;
} MotionEventGate;

typedef struct {
    int event_emitted;
    MotionEventPhase phase;
    uint64_t event_count;
    uint64_t cooldown_remaining_ms;
} MotionEventDecision;

MotionEventGateStatus motion_event_gate_update(MotionEventGate *gate,
                                               int motion_detected,
                                               uint64_t now_ms,
                                               uint64_t cooldown_ms,
                                               MotionEventDecision *decision);

void motion_event_gate_reset(MotionEventGate *gate);

const char *motion_event_phase_string(MotionEventPhase phase);

const char *motion_event_gate_status_string(MotionEventGateStatus status);

#endif