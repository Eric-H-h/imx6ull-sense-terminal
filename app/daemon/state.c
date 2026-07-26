#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t monotonic_ns(void)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    return ((uint64_t)now.tv_sec * 1000000000ULL) + (uint64_t)now.tv_nsec;
}

static void copy_text(char *destination, size_t destination_size,
                      const char *source)
{
    if (destination_size == 0) {
        return;
    }
    if (source == NULL) {
        source = "";
    }
    snprintf(destination, destination_size, "%s", source);
}

int state_init(AppState *state)
{
    memset(state, 0, sizeof(*state));
    if (pthread_mutex_init(&state->lock, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&state->frame_ready, NULL) != 0) {
        pthread_mutex_destroy(&state->lock);
        return -1;
    }
    if (pthread_cond_init(&state->clients_done, NULL) != 0) {
        pthread_cond_destroy(&state->frame_ready);
        pthread_mutex_destroy(&state->lock);
        return -1;
    }

    state->degraded = 1;
    copy_text(state->device, sizeof(state->device), "none");
    copy_text(state->last_error, sizeof(state->last_error),
              "camera not initialized");
    return 0;
}

void state_destroy(AppState *state)
{
    free(state->frame);
    pthread_cond_destroy(&state->clients_done);
    pthread_cond_destroy(&state->frame_ready);
    pthread_mutex_destroy(&state->lock);
}

void state_request_stop(AppState *state)
{
    pthread_mutex_lock(&state->lock);
    state->stop = 1;
    pthread_cond_broadcast(&state->frame_ready);
    pthread_mutex_unlock(&state->lock);
}

int state_should_stop(AppState *state)
{
    int stop;

    pthread_mutex_lock(&state->lock);
    stop = state->stop;
    pthread_mutex_unlock(&state->lock);
    return stop || g_signal_stop;
}

void state_wait_for_clients(AppState *state)
{
    pthread_mutex_lock(&state->lock);
    while (state->worker_count > 0) {
        pthread_cond_wait(&state->clients_done, &state->lock);
    }
    pthread_mutex_unlock(&state->lock);
}

void state_set_capture_active(AppState *state, const char *device,
                              unsigned int width, unsigned int height)
{
    pthread_mutex_lock(&state->lock);
    ++state->capture_generation;
    state->degraded = 0;
    state->width = width;
    state->height = height;
    state->fps = 0.0;
    state->capture_base_count = state->frame_count;
    state->capture_started_ns = monotonic_ns();
    state->last_error[0] = '\0';
    copy_text(state->device, sizeof(state->device), device);
    pthread_cond_broadcast(&state->frame_ready);
    pthread_mutex_unlock(&state->lock);
}

void state_set_degraded(AppState *state, const char *device,
                        const char *error)
{
    pthread_mutex_lock(&state->lock);
    state->degraded = 1;
    state->fps = 0.0;
    state->motion_state = 0;
    state->motion_score = 0.0;
    state->motion_sample_fps = 0.0;
    if ((device != NULL) && (device[0] != '\0')) {
        copy_text(state->device, sizeof(state->device), device);
    }
    copy_text(state->last_error, sizeof(state->last_error), error);
    pthread_cond_broadcast(&state->frame_ready);
    pthread_mutex_unlock(&state->lock);
}

int state_publish_frame(AppState *state, const void *frame, size_t frame_size)
{
    unsigned char *resized;
    uint64_t now;
    uint64_t elapsed;
    uint64_t captured;

    pthread_mutex_lock(&state->lock);
    if (frame_size > state->frame_capacity) {
        resized = realloc(state->frame, frame_size);
        if (resized == NULL) {
            pthread_mutex_unlock(&state->lock);
            return -1;
        }
        state->frame = resized;
        state->frame_capacity = frame_size;
    }

    memcpy(state->frame, frame, frame_size);
    state->frame_size = frame_size;
    ++state->frame_sequence;
    state->frame_generation = state->capture_generation;
    ++state->frame_count;

    now = monotonic_ns();
    elapsed = now - state->capture_started_ns;
    captured = state->frame_count - state->capture_base_count;
    if (elapsed > 0) {
        state->fps = ((double)captured * 1000000000.0) / (double)elapsed;
    }

    pthread_cond_broadcast(&state->frame_ready);
    pthread_mutex_unlock(&state->lock);
    return 0;
}

int state_wait_frame(AppState *state, uint64_t *last_sequence,
                     unsigned char **buffer, size_t *capacity,
                     size_t *frame_size)
{
    struct timespec deadline;
    unsigned char *resized;
    int wait_result;

    clock_gettime(CLOCK_REALTIME, &deadline);
    ++deadline.tv_sec;

    pthread_mutex_lock(&state->lock);
    while ((state->frame_sequence <= *last_sequence) && !state->stop &&
           !g_signal_stop) {
        wait_result = pthread_cond_timedwait(&state->frame_ready, &state->lock,
                                             &deadline);
        if (wait_result == ETIMEDOUT) {
            pthread_mutex_unlock(&state->lock);
            return 0;
        }
        if (wait_result != 0) {
            pthread_mutex_unlock(&state->lock);
            return -1;
        }
    }

    if (state->stop || g_signal_stop) {
        pthread_mutex_unlock(&state->lock);
        return -1;
    }

    if (state->frame_size > *capacity) {
        resized = realloc(*buffer, state->frame_size);
        if (resized == NULL) {
            pthread_mutex_unlock(&state->lock);
            return -2;
        }
        *buffer = resized;
        *capacity = state->frame_size;
    }

    memcpy(*buffer, state->frame, state->frame_size);
    *frame_size = state->frame_size;
    *last_sequence = state->frame_sequence;
    pthread_mutex_unlock(&state->lock);
    return 1;
}

JpegSnapshotStatus state_wait_jpeg_snapshot(AppState *state,
                                            JpegSnapshot *snapshot)
{
    struct timespec deadline;
    unsigned char *resized;
    int wait_result;

    if ((state == NULL) || (snapshot == NULL)) {
        return JPEG_SNAPSHOT_ERROR;
    }

    clock_gettime(CLOCK_REALTIME, &deadline);
    ++deadline.tv_sec;

    pthread_mutex_lock(&state->lock);
    while ((state->frame_sequence <= snapshot->sequence) &&
           !state->degraded && !state->stop && !g_signal_stop) {
        wait_result = pthread_cond_timedwait(&state->frame_ready, &state->lock,
                                             &deadline);
        if (wait_result == ETIMEDOUT) {
            pthread_mutex_unlock(&state->lock);
            return JPEG_SNAPSHOT_TIMEOUT;
        }
        if (wait_result != 0) {
            pthread_mutex_unlock(&state->lock);
            return JPEG_SNAPSHOT_ERROR;
        }
    }

    if (state->stop || g_signal_stop) {
        pthread_mutex_unlock(&state->lock);
        return JPEG_SNAPSHOT_STOPPED;
    }
    if (state->degraded ||
        (state->frame_generation != state->capture_generation)) {
        snapshot->sequence = state->frame_sequence;
        pthread_mutex_unlock(&state->lock);
        return JPEG_SNAPSHOT_UNAVAILABLE;
    }

    if (state->frame_size > snapshot->capacity) {
        resized = realloc(snapshot->data, state->frame_size);
        if (resized == NULL) {
            pthread_mutex_unlock(&state->lock);
            return JPEG_SNAPSHOT_NO_MEMORY;
        }
        snapshot->data = resized;
        snapshot->capacity = state->frame_size;
    }

    memcpy(snapshot->data, state->frame, state->frame_size);
    snapshot->size = state->frame_size;
    snapshot->sequence = state->frame_sequence;
    snapshot->capture_generation = state->frame_generation;
    pthread_mutex_unlock(&state->lock);
    return JPEG_SNAPSHOT_READY;
}

void state_release_jpeg_snapshot(JpegSnapshot *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    free(snapshot->data);
    memset(snapshot, 0, sizeof(*snapshot));
}

void state_worker_delta(AppState *state, int delta)
{
    pthread_mutex_lock(&state->lock);
    if (delta > 0) {
        state->worker_count += (unsigned int)delta;
    } else if ((delta < 0) && (state->worker_count > 0)) {
        --state->worker_count;
        if (state->worker_count == 0) {
            pthread_cond_broadcast(&state->clients_done);
        }
    }
    pthread_mutex_unlock(&state->lock);
}

void state_stream_client_delta(AppState *state, int delta)
{
    pthread_mutex_lock(&state->lock);
    if (delta > 0) {
        state->client_count += (unsigned int)delta;
    } else if ((delta < 0) && (state->client_count > 0)) {
        --state->client_count;
    }
    pthread_mutex_unlock(&state->lock);
}

void state_configure_motion(AppState *state, int enabled)
{
    pthread_mutex_lock(&state->lock);
    state->motion_enabled = enabled != 0;
    state->motion_state = 0;
    state->motion_score = 0.0;
    state->motion_sample_fps = 0.0;
    state->event_count = 0U;
    pthread_mutex_unlock(&state->lock);
}

void state_update_motion(AppState *state, int motion_detected,
                         double score, double sample_fps,
                         uint64_t event_count)
{
    pthread_mutex_lock(&state->lock);
    if (state->motion_enabled) {
        state->motion_state = motion_detected != 0;
        state->motion_score = score;
        state->motion_sample_fps = sample_fps;
        state->event_count = event_count;
    }
    pthread_mutex_unlock(&state->lock);
}

void state_reset_motion_sample(AppState *state)
{
    pthread_mutex_lock(&state->lock);
    state->motion_state = 0;
    state->motion_score = 0.0;
    state->motion_sample_fps = 0.0;
    pthread_mutex_unlock(&state->lock);
}

void state_snapshot(AppState *state, StatusSnapshot *snapshot)
{
    pthread_mutex_lock(&state->lock);
    snapshot->degraded = state->degraded;
    snapshot->width = state->width;
    snapshot->height = state->height;
    snapshot->fps = state->fps;
    snapshot->frame_count = state->frame_count;
    snapshot->client_count = state->client_count;
    snapshot->motion_enabled = state->motion_enabled;
    snapshot->motion_state = state->motion_state;
    snapshot->motion_score = state->motion_score;
    snapshot->motion_sample_fps = state->motion_sample_fps;
    snapshot->event_count = state->event_count;
    copy_text(snapshot->device, sizeof(snapshot->device), state->device);
    copy_text(snapshot->last_error, sizeof(snapshot->last_error),
              state->last_error);
    pthread_mutex_unlock(&state->lock);
}
