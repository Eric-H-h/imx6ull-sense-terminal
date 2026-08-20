#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

volatile sig_atomic_t g_signal_stop = 0;

static int failure_count = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                             \
            ++failure_count;                                                 \
        }                                                                    \
    } while (0)

static void test_component_health_is_derived(void)
{
    AppState state;
    StatusSnapshot snapshot;
    struct timespec delay = {0, 1000000L};
    double first_uptime;

    CHECK(state_init(&state) == 0);
    state_configure_motion(&state, 1);
    state_snapshot(&state, &snapshot);
    CHECK(snapshot.degraded == 1);
    CHECK(snapshot.camera_state == SENSE_CAMERA_INITIALIZING);
    CHECK(snapshot.event_log_state == SENSE_EVENT_LOG_OK);
    CHECK(strcmp(snapshot.last_error, "camera not initialized") == 0);
    CHECK(snapshot.last_error_at_ms > 0U);
    first_uptime = snapshot.uptime_seconds;

    state_set_capture_active(&state, "/dev/video-test", 640U, 480U);
    state_snapshot(&state, &snapshot);
    CHECK(snapshot.degraded == 0);
    CHECK(snapshot.camera_state == SENSE_CAMERA_ACTIVE);
    CHECK(snapshot.last_error[0] == '\0');
    CHECK(snapshot.last_error_at_ms == 0U);

    state_set_event_log_unavailable(&state, "event log write failed");
    state_snapshot(&state, &snapshot);
    CHECK(snapshot.degraded == 1);
    CHECK(snapshot.camera_state == SENSE_CAMERA_ACTIVE);
    CHECK(snapshot.event_log_state == SENSE_EVENT_LOG_UNAVAILABLE);
    CHECK(strcmp(snapshot.last_error, "event log write failed") == 0);

    state_set_camera_unavailable(&state, "/dev/video-test",
                                 "camera missing");
    state_set_event_log_ok(&state);
    state_snapshot(&state, &snapshot);
    CHECK(snapshot.degraded == 1);
    CHECK(snapshot.event_log_state == SENSE_EVENT_LOG_OK);
    CHECK(strcmp(snapshot.last_error, "camera missing") == 0);

    state_set_capture_active(&state, "/dev/video-test", 640U, 480U);
    nanosleep(&delay, NULL);
    state_snapshot(&state, &snapshot);
    CHECK(snapshot.degraded == 0);
    CHECK(snapshot.uptime_seconds >= first_uptime);

    state_destroy(&state);
}

static void test_disabled_event_log_does_not_degrade_health(void)
{
    AppState state;
    StatusSnapshot snapshot;

    CHECK(state_init(&state) == 0);
    state_configure_motion(&state, 0);
    state_set_capture_active(&state, "/dev/video-test", 640U, 480U);
    state_set_event_log_unavailable(&state, "ignored");
    state_snapshot(&state, &snapshot);

    CHECK(snapshot.degraded == 0);
    CHECK(snapshot.event_log_state == SENSE_EVENT_LOG_DISABLED);
    CHECK(snapshot.last_error[0] == '\0');
    state_destroy(&state);
}

int main(void)
{
    test_component_health_is_derived();
    test_disabled_event_log_does_not_degrade_health();

    if (failure_count != 0) {
        fprintf(stderr, "state tests: %d failure(s)\n", failure_count);
        return 1;
    }

    puts("state tests: PASS");
    return 0;
}
