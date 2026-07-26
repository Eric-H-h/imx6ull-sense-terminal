#include "motion_event_gate.h"

#include <stdint.h>
#include <stdio.h>

static int failure_count = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                             \
            ++failure_count;                                                 \
        }                                                                    \
    } while (0)

static const uint64_t cooldown_ms = 3000U;

static MotionEventDecision update_gate(MotionEventGate *gate,
                                       int motion_detected,
                                       uint64_t now_ms)
{
    MotionEventDecision decision = {0};

    CHECK(motion_event_gate_update(gate, motion_detected, now_ms,
                                   cooldown_ms, &decision) ==
          MOTION_EVENT_GATE_OK);
    return decision;
}

static void test_idle_without_motion(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision = update_gate(&gate, 0, 1000U);

    CHECK(decision.event_emitted == 0);
    CHECK(decision.phase == MOTION_EVENT_IDLE);
    CHECK(decision.event_count == 0U);
    CHECK(decision.cooldown_remaining_ms == 0U);
}

static void test_first_motion_emits_event(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision = update_gate(&gate, 1, 1000U);

    CHECK(decision.event_emitted == 1);
    CHECK(decision.phase == MOTION_EVENT_COOLDOWN);
    CHECK(decision.event_count == 1U);
    CHECK(decision.cooldown_remaining_ms == cooldown_ms);
    CHECK(gate.cooldown_started_ms == 1000U);
}

static void test_cooldown_suppresses_duplicate_events(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision;

    (void)update_gate(&gate, 1, 1000U);
    decision = update_gate(&gate, 1, 2000U);

    CHECK(decision.event_emitted == 0);
    CHECK(decision.phase == MOTION_EVENT_COOLDOWN);
    CHECK(decision.event_count == 1U);
    CHECK(decision.cooldown_remaining_ms == 2000U);
}

static void test_exact_cooldown_boundary_emits_again(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision;

    (void)update_gate(&gate, 1, 1000U);
    decision = update_gate(&gate, 1, 4000U);

    CHECK(decision.event_emitted == 1);
    CHECK(decision.phase == MOTION_EVENT_COOLDOWN);
    CHECK(decision.event_count == 2U);
    CHECK(decision.cooldown_remaining_ms == cooldown_ms);
    CHECK(gate.cooldown_started_ms == 4000U);
}

static void test_continuous_motion_is_rate_limited(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision;

    decision = update_gate(&gate, 1, 0U);
    CHECK(decision.event_emitted == 1);
    decision = update_gate(&gate, 1, 1000U);
    CHECK(decision.event_emitted == 0);
    decision = update_gate(&gate, 1, 3000U);
    CHECK(decision.event_emitted == 1);
    decision = update_gate(&gate, 1, 5999U);
    CHECK(decision.event_emitted == 0);
    decision = update_gate(&gate, 1, 6000U);
    CHECK(decision.event_emitted == 1);
    CHECK(decision.event_count == 3U);
}

static void test_motion_stops_and_later_restarts(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision;

    (void)update_gate(&gate, 1, 1000U);
    decision = update_gate(&gate, 0, 2000U);
    CHECK(decision.phase == MOTION_EVENT_COOLDOWN);
    CHECK(decision.event_emitted == 0);

    decision = update_gate(&gate, 0, 4000U);
    CHECK(decision.phase == MOTION_EVENT_IDLE);
    CHECK(decision.event_emitted == 0);
    CHECK(decision.event_count == 1U);

    decision = update_gate(&gate, 1, 4500U);
    CHECK(decision.phase == MOTION_EVENT_COOLDOWN);
    CHECK(decision.event_emitted == 1);
    CHECK(decision.event_count == 2U);
}

static void test_reset_clears_state(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision;

    (void)update_gate(&gate, 1, 1000U);
    motion_event_gate_reset(&gate);

    CHECK(gate.phase == MOTION_EVENT_IDLE);
    CHECK(gate.cooldown_started_ms == 0U);
    CHECK(gate.event_count == 0U);

    decision = update_gate(&gate, 1, 1100U);
    CHECK(decision.event_emitted == 1);
    CHECK(decision.event_count == 1U);

    motion_event_gate_reset(NULL);
}

static void test_clock_rollback_does_not_emit(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision;

    (void)update_gate(&gate, 1, 5000U);
    decision = update_gate(&gate, 1, 4000U);

    CHECK(decision.event_emitted == 0);
    CHECK(decision.phase == MOTION_EVENT_COOLDOWN);
    CHECK(decision.event_count == 1U);
    CHECK(decision.cooldown_remaining_ms == cooldown_ms);
    CHECK(gate.cooldown_started_ms == 5000U);
}

static void test_event_count_saturates(void)
{
    MotionEventGate gate = {
        .phase = MOTION_EVENT_IDLE,
        .event_count = UINT64_MAX
    };
    MotionEventDecision decision = update_gate(&gate, 1, 1000U);

    CHECK(decision.event_emitted == 1);
    CHECK(decision.event_count == UINT64_MAX);
}

static void test_invalid_arguments_and_state(void)
{
    MotionEventGate gate = {0};
    MotionEventDecision decision = {0};

    CHECK(motion_event_gate_update(NULL, 0, 0U, cooldown_ms, &decision) ==
          MOTION_EVENT_GATE_INVALID_ARGUMENT);
    CHECK(motion_event_gate_update(&gate, 0, 0U, cooldown_ms, NULL) ==
          MOTION_EVENT_GATE_INVALID_ARGUMENT);
    CHECK(motion_event_gate_update(&gate, -1, 0U, cooldown_ms, &decision) ==
          MOTION_EVENT_GATE_INVALID_ARGUMENT);
    CHECK(motion_event_gate_update(&gate, 2, 0U, cooldown_ms, &decision) ==
          MOTION_EVENT_GATE_INVALID_ARGUMENT);
    CHECK(motion_event_gate_update(&gate, 0, 0U, 0U, &decision) ==
          MOTION_EVENT_GATE_INVALID_ARGUMENT);

    gate.phase = (MotionEventPhase)99;
    CHECK(motion_event_gate_update(&gate, 0, 0U, cooldown_ms, &decision) ==
          MOTION_EVENT_GATE_INVALID_STATE);
}

static void test_status_strings(void)
{
    CHECK(motion_event_phase_string(MOTION_EVENT_IDLE) != NULL);
    CHECK(motion_event_phase_string(MOTION_EVENT_COOLDOWN) != NULL);
    CHECK(motion_event_phase_string((MotionEventPhase)99) != NULL);
    CHECK(motion_event_gate_status_string(MOTION_EVENT_GATE_OK) != NULL);
    CHECK(motion_event_gate_status_string(
              MOTION_EVENT_GATE_INVALID_ARGUMENT) != NULL);
    CHECK(motion_event_gate_status_string(
              MOTION_EVENT_GATE_INVALID_STATE) != NULL);
    CHECK(motion_event_gate_status_string((MotionEventGateStatus)-99) != NULL);
}

int main(void)
{
    test_idle_without_motion();
    test_first_motion_emits_event();
    test_cooldown_suppresses_duplicate_events();
    test_exact_cooldown_boundary_emits_again();
    test_continuous_motion_is_rate_limited();
    test_motion_stops_and_later_restarts();
    test_reset_clears_state();
    test_clock_rollback_does_not_emit();
    test_event_count_saturates();
    test_invalid_arguments_and_state();
    test_status_strings();

    if (failure_count != 0) {
        fprintf(stderr, "motion event gate tests: %d failure(s)\n",
                failure_count);
        return 1;
    }

    puts("motion event gate tests: PASS");
    return 0;
}