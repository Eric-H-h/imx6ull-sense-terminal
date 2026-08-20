#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include "motion_pipeline.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define NANOSECONDS_PER_SECOND 1000000000ULL

static uint64_t clock_ns(clockid_t clock_id)
{
    struct timespec now;

    clock_gettime(clock_id, &now);
    return ((uint64_t)now.tv_sec * 1000000000ULL) + (uint64_t)now.tv_nsec;
}

static uint64_t realtime_ms(void)
{
    return clock_ns(CLOCK_REALTIME) / 1000000ULL;
}

static int wait_until_sample(AppState *state, uint64_t deadline_ns)
{
    struct timespec deadline = {
        .tv_sec = (time_t)(deadline_ns / 1000000000ULL),
        .tv_nsec = (long)(deadline_ns % 1000000000ULL)
    };
    int result;

    while (!state_should_stop(state)) {
        result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                 &deadline, NULL);
        if (result == 0) {
            return state_should_stop(state) ? -1 : 0;
        }
        if (result != EINTR) {
            return -1;
        }
    }
    return -1;
}

static void advance_sample_deadline(uint64_t *deadline_ns,
                                    uint64_t interval_ns)
{
    uint64_t now = clock_ns(CLOCK_MONOTONIC);

    do {
        *deadline_ns += interval_ns;
    } while (*deadline_ns <= now);
}

static MotionPipelineConfig make_pipeline_config(const SenseConfig *config)
{
    MotionPipelineConfig pipeline_config = {
        .jpeg_scale_denom = config->motion_jpeg_scale_denom,
        .thresholds = {
            .pixel_delta_threshold = config->motion_pixel_delta_threshold,
            .changed_ratio_threshold =
                config->motion_changed_ratio_threshold
        },
        .cooldown_ms = config->motion_cooldown_ms,
        .event_log_path = config->event_log
    };

    return pipeline_config;
}

static void report_pipeline_error(MotionPipelineStatus status)
{
    if (status == MOTION_PIPELINE_EVENT_LOG_ERROR) {
        fprintf(stderr, "motion pipeline: %s: %s\n",
                motion_pipeline_status_string(status), strerror(errno));
    } else {
        fprintf(stderr, "motion pipeline: %s\n",
                motion_pipeline_status_string(status));
    }
}

static void update_event_log_health(AppState *state,
                                    MotionPipelineStatus status,
                                    const MotionPipelineResult *result)
{
    if (status == MOTION_PIPELINE_EVENT_LOG_ERROR) {
        char error[SENSE_ERROR_MAX];
        int saved_errno = errno;

        snprintf(error, sizeof(error), "event log write failed: %s",
                 strerror(saved_errno));
        state_set_event_log_unavailable(state, error);
        errno = saved_errno;
    } else if ((status == MOTION_PIPELINE_OK) &&
               (result->event_logged != 0)) {
        state_set_event_log_ok(state);
    }
}

static double observe_sample_rate(uint64_t now_ns,
                                  uint64_t *started_ns,
                                  uint64_t *sample_count)
{
    if (*sample_count == 0U) {
        *started_ns = now_ns;
    }
    ++*sample_count;
    if ((*sample_count <= 1U) || (now_ns <= *started_ns)) {
        return 0.0;
    }

    return ((double)(*sample_count - 1U) *
            (double)NANOSECONDS_PER_SECOND) /
           (double)(now_ns - *started_ns);
}

void *motion_worker_thread_main(void *argument)
{
    MotionWorkerContext *context = argument;
    MotionPipelineConfig pipeline_config =
        make_pipeline_config(context->config);
    MotionPipeline pipeline = {0};
    MotionPipelineResult pipeline_result;
    JpegSnapshot snapshot = {0};
    uint64_t last_generation = 0U;
    uint64_t sample_rate_started_ns = 0U;
    uint64_t completed_samples = 0U;
    uint64_t sample_interval_ns;
    uint64_t next_sample_ns = clock_ns(CLOCK_MONOTONIC);
    MotionPipelineStatus last_error = MOTION_PIPELINE_OK;

    if (!context->config->motion_enabled) {
        puts("motion worker: disabled");
        state_reset_motion_sample(context->state);
        return NULL;
    }
    sample_interval_ns =
        NANOSECONDS_PER_SECOND / context->config->motion_sample_fps;

    printf("motion worker: %u fps, grayscale 1/%u, "
           "pixel delta %u, changed ratio %.2f%%, cooldown %u ms\n",
           context->config->motion_sample_fps,
           context->config->motion_jpeg_scale_denom,
           context->config->motion_pixel_delta_threshold,
           context->config->motion_changed_ratio_threshold * 100.0,
           context->config->motion_cooldown_ms);

    while (!state_should_stop(context->state)) {
        JpegSnapshotStatus snapshot_status;

        if (wait_until_sample(context->state, next_sample_ns) < 0) {
            break;
        }

        snapshot_status =
            state_wait_jpeg_snapshot(context->state, &snapshot);
        if (snapshot_status == JPEG_SNAPSHOT_STOPPED) {
            break;
        }
        if (snapshot_status == JPEG_SNAPSHOT_UNAVAILABLE) {
            motion_pipeline_reset_baseline(&pipeline);
            last_generation = 0U;
            sample_rate_started_ns = 0U;
            completed_samples = 0U;
            state_reset_motion_sample(context->state);
            advance_sample_deadline(&next_sample_ns, sample_interval_ns);
            continue;
        }
        if (snapshot_status == JPEG_SNAPSHOT_TIMEOUT) {
            motion_pipeline_reset_baseline(&pipeline);
            last_generation = 0U;
            sample_rate_started_ns = 0U;
            completed_samples = 0U;
            state_reset_motion_sample(context->state);
            advance_sample_deadline(&next_sample_ns, sample_interval_ns);
            continue;
        }
        if (snapshot_status != JPEG_SNAPSHOT_READY) {
            fprintf(stderr, "motion worker: JPEG snapshot failed (%d)\n",
                    snapshot_status);
            state_reset_motion_sample(context->state);
            advance_sample_deadline(&next_sample_ns, sample_interval_ns);
            continue;
        }

        if (snapshot.capture_generation != last_generation) {
            motion_pipeline_reset_baseline(&pipeline);
            last_generation = snapshot.capture_generation;
            sample_rate_started_ns = 0U;
            completed_samples = 0U;
            state_reset_motion_sample(context->state);
        }

        {
            uint64_t sample_monotonic_ns = clock_ns(CLOCK_MONOTONIC);
            MotionJpegSample sample = {
                .jpeg_data = snapshot.data,
                .jpeg_size = snapshot.size,
                .sequence = snapshot.sequence,
                .monotonic_ms = sample_monotonic_ns / 1000000ULL,
                .realtime_ms = realtime_ms()
            };
            MotionPipelineStatus pipeline_status =
                motion_pipeline_process_jpeg(&pipeline,
                                             &pipeline_config,
                                             &sample,
                                             &pipeline_result);

            update_event_log_health(context->state, pipeline_status,
                                    &pipeline_result);

            if ((pipeline_status == MOTION_PIPELINE_OK) ||
                (pipeline_status == MOTION_PIPELINE_EVENT_LOG_ERROR)) {
                double sample_fps = observe_sample_rate(
                    sample_monotonic_ns,
                    &sample_rate_started_ns,
                    &completed_samples);

                state_update_motion(context->state,
                                    pipeline_result.motion_detected,
                                    pipeline_result.score,
                                    sample_fps,
                                    pipeline_result.event_count);
            } else {
                state_reset_motion_sample(context->state);
            }

            if (pipeline_status != MOTION_PIPELINE_OK) {
                if (pipeline_status != last_error) {
                    report_pipeline_error(pipeline_status);
                }
                last_error = pipeline_status;
            } else {
                last_error = MOTION_PIPELINE_OK;
                if (pipeline_result.event_logged != 0) {
                    printf("motion event: sequence %llu, score %.4f, "
                           "event_count %llu\n",
                           (unsigned long long)pipeline_result.sequence,
                           pipeline_result.score,
                           (unsigned long long)pipeline_result.event_count);
                }
            }
        }

        advance_sample_deadline(&next_sample_ns, sample_interval_ns);
    }

    state_reset_motion_sample(context->state);
    state_release_jpeg_snapshot(&snapshot);
    motion_pipeline_destroy(&pipeline);
    return NULL;
}
