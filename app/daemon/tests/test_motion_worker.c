#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <jpeglib.h>

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

static void create_test_jpeg(int changed,
                             unsigned char **jpeg_data,
                             unsigned long *jpeg_size)
{
    struct jpeg_compress_struct encoder;
    struct jpeg_error_mgr error;
    unsigned char row[64];

    encoder.err = jpeg_std_error(&error);
    jpeg_create_compress(&encoder);
    jpeg_mem_dest(&encoder, jpeg_data, jpeg_size);

    encoder.image_width = 64U;
    encoder.image_height = 64U;
    encoder.input_components = 1;
    encoder.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&encoder);
    jpeg_set_quality(&encoder, 100, TRUE);
    jpeg_start_compress(&encoder, TRUE);

    while (encoder.next_scanline < encoder.image_height) {
        JSAMPROW row_pointer = row;

        for (unsigned int column = 0U; column < 64U; ++column) {
            row[column] = (changed && (column < 32U)) ? 220U : 32U;
        }
        jpeg_write_scanlines(&encoder, &row_pointer, 1U);
    }

    jpeg_finish_compress(&encoder);
    jpeg_destroy_compress(&encoder);
}

static void sleep_ms(long milliseconds)
{
    struct timespec delay = {
        .tv_sec = milliseconds / 1000L,
        .tv_nsec = (milliseconds % 1000L) * 1000000L
    };

    while ((nanosleep(&delay, &delay) != 0) && !g_signal_stop) {
    }
}

static size_t count_log_lines(const char *path)
{
    FILE *file = fopen(path, "rb");
    size_t lines = 0U;
    int value;

    if (file == NULL) {
        return 0U;
    }
    while ((value = fgetc(file)) != EOF) {
        if (value == '\n') {
            ++lines;
        }
    }
    fclose(file);
    return lines;
}

static void test_snapshot_tracks_capture_generation(void)
{
    static const unsigned char first[] = {1U, 2U, 3U};
    static const unsigned char second[] = {4U, 5U, 6U, 7U};
    AppState state;
    JpegSnapshot snapshot = {0};
    uint64_t first_generation;

    CHECK(state_init(&state) == 0);
    CHECK(state_wait_jpeg_snapshot(&state, &snapshot) ==
          JPEG_SNAPSHOT_UNAVAILABLE);

    state_set_capture_active(&state, "/dev/video-test", 64U, 64U);
    CHECK(state_publish_frame(&state, first, sizeof(first)) == 0);
    CHECK(state_wait_jpeg_snapshot(&state, &snapshot) ==
          JPEG_SNAPSHOT_READY);
    CHECK(snapshot.size == sizeof(first));
    CHECK(memcmp(snapshot.data, first, sizeof(first)) == 0);
    first_generation = snapshot.capture_generation;
    CHECK(first_generation != 0U);

    state_set_camera_unavailable(&state, "/dev/video-test",
                                 "camera missing");
    CHECK(state_wait_jpeg_snapshot(&state, &snapshot) ==
          JPEG_SNAPSHOT_UNAVAILABLE);

    state_set_capture_active(&state, "/dev/video-test", 64U, 64U);
    CHECK(state_publish_frame(&state, second, sizeof(second)) == 0);
    CHECK(state_wait_jpeg_snapshot(&state, &snapshot) ==
          JPEG_SNAPSHOT_READY);
    CHECK(snapshot.capture_generation != first_generation);
    CHECK(snapshot.size == sizeof(second));
    CHECK(memcmp(snapshot.data, second, sizeof(second)) == 0);

    state_request_stop(&state);
    CHECK(state_wait_jpeg_snapshot(&state, &snapshot) ==
          JPEG_SNAPSHOT_STOPPED);
    state_release_jpeg_snapshot(&snapshot);
    state_release_jpeg_snapshot(&snapshot);
    state_release_jpeg_snapshot(NULL);
    state_destroy(&state);
}

static void test_worker_logs_motion_and_rebuilds_after_recovery(void)
{
    unsigned char *base_jpeg = NULL;
    unsigned char *changed_jpeg = NULL;
    unsigned long base_size = 0U;
    unsigned long changed_size = 0U;
    char path[192];
    SenseConfig config;
    AppState state;
    StatusSnapshot status;
    MotionWorkerContext context;
    pthread_t worker;
    int length;

    create_test_jpeg(0, &base_jpeg, &base_size);
    create_test_jpeg(1, &changed_jpeg, &changed_size);
    CHECK((base_jpeg != NULL) && (changed_jpeg != NULL));
    if ((base_jpeg == NULL) || (changed_jpeg == NULL)) {
        free(base_jpeg);
        free(changed_jpeg);
        return;
    }

    length = snprintf(path,
                      sizeof(path),
                      "/tmp/imx6ull-motion-worker-%ld.jsonl",
                      (long)getpid());
    CHECK((length > 0) && ((size_t)length < sizeof(path)));
    (void)unlink(path);

    memset(&config, 0, sizeof(config));
    snprintf(config.event_log, sizeof(config.event_log), "%s", path);
    config.motion_enabled = 1;
    config.motion_sample_fps = 3U;
    config.motion_jpeg_scale_denom = 4U;
    config.motion_pixel_delta_threshold = 25U;
    config.motion_changed_ratio_threshold = 0.05;
    config.motion_cooldown_ms = 300U;

    CHECK(state_init(&state) == 0);
    state_configure_motion(&state, config.motion_enabled);
    context.config = &config;
    context.state = &state;

    state_set_capture_active(&state, "/dev/video-test", 64U, 64U);
    CHECK(state_publish_frame(&state, base_jpeg, (size_t)base_size) == 0);
    CHECK(pthread_create(&worker, NULL, motion_worker_thread_main,
                         &context) == 0);

    sleep_ms(450L);
    CHECK(state_publish_frame(&state, changed_jpeg,
                              (size_t)changed_size) == 0);
    sleep_ms(450L);
    state_snapshot(&state, &status);
    CHECK(status.motion_enabled == 1);
    CHECK(status.motion_state == 1);
    CHECK(status.motion_score > 0.40);
    CHECK(status.motion_sample_fps > 0.0);
    CHECK(status.event_count == 1U);

    state_set_camera_unavailable(&state, "/dev/video-test",
                                 "camera missing");
    state_set_capture_active(&state, "/dev/video-test", 64U, 64U);
    CHECK(state_publish_frame(&state, base_jpeg, (size_t)base_size) == 0);
    sleep_ms(450L);
    state_snapshot(&state, &status);
    CHECK(status.motion_state == 0);
    CHECK(status.motion_score == 0.0);
    CHECK(status.event_count == 1U);

    state_request_stop(&state);
    CHECK(pthread_join(worker, NULL) == 0);
    CHECK(count_log_lines(path) == 1U);

    state_destroy(&state);
    free(base_jpeg);
    free(changed_jpeg);
    (void)unlink(path);
}

static void test_worker_reports_event_log_failure(void)
{
    unsigned char *base_jpeg = NULL;
    unsigned char *changed_jpeg = NULL;
    unsigned long base_size = 0U;
    unsigned long changed_size = 0U;
    SenseConfig config;
    AppState state;
    StatusSnapshot status;
    MotionWorkerContext context;
    pthread_t worker;

    if (access("/dev/full", W_OK) != 0) {
        return;
    }

    create_test_jpeg(0, &base_jpeg, &base_size);
    create_test_jpeg(1, &changed_jpeg, &changed_size);
    CHECK((base_jpeg != NULL) && (changed_jpeg != NULL));
    if ((base_jpeg == NULL) || (changed_jpeg == NULL)) {
        free(base_jpeg);
        free(changed_jpeg);
        return;
    }

    memset(&config, 0, sizeof(config));
    snprintf(config.event_log, sizeof(config.event_log), "/dev/full");
    config.motion_enabled = 1;
    config.motion_sample_fps = 3U;
    config.motion_jpeg_scale_denom = 4U;
    config.motion_pixel_delta_threshold = 25U;
    config.motion_changed_ratio_threshold = 0.05;
    config.motion_cooldown_ms = 300U;

    CHECK(state_init(&state) == 0);
    state_configure_motion(&state, config.motion_enabled);
    context.config = &config;
    context.state = &state;

    state_set_capture_active(&state, "/dev/video-test", 64U, 64U);
    CHECK(state_publish_frame(&state, base_jpeg, (size_t)base_size) == 0);
    CHECK(pthread_create(&worker, NULL, motion_worker_thread_main,
                         &context) == 0);

    sleep_ms(450L);
    CHECK(state_publish_frame(&state, changed_jpeg,
                              (size_t)changed_size) == 0);
    sleep_ms(450L);
    state_snapshot(&state, &status);

    CHECK(status.degraded == 1);
    CHECK(status.camera_state == SENSE_CAMERA_ACTIVE);
    CHECK(status.event_log_state == SENSE_EVENT_LOG_UNAVAILABLE);
    CHECK(strstr(status.last_error, "event log write failed") != NULL);
    CHECK(status.event_count == 1U);

    state_request_stop(&state);
    CHECK(pthread_join(worker, NULL) == 0);
    state_destroy(&state);
    free(base_jpeg);
    free(changed_jpeg);
}

static void test_disabled_worker_exits_cleanly(void)
{
    SenseConfig config = {0};
    AppState state;
    StatusSnapshot status;
    MotionWorkerContext context;
    pthread_t worker;

    CHECK(state_init(&state) == 0);
    state_configure_motion(&state, config.motion_enabled);
    context.config = &config;
    context.state = &state;

    CHECK(pthread_create(&worker, NULL, motion_worker_thread_main,
                         &context) == 0);
    CHECK(pthread_join(worker, NULL) == 0);
    state_snapshot(&state, &status);
    CHECK(status.motion_enabled == 0);
    CHECK(status.motion_state == 0);
    CHECK(status.event_count == 0U);
    state_destroy(&state);
}

int main(void)
{
    test_snapshot_tracks_capture_generation();
    test_worker_logs_motion_and_rebuilds_after_recovery();
    test_worker_reports_event_log_failure();
    test_disabled_worker_exits_cleanly();

    if (failure_count != 0) {
        fprintf(stderr, "motion worker tests: %d failure(s)\n",
                failure_count);
        return 1;
    }

    puts("motion worker tests: PASS");
    return 0;
}
