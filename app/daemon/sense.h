#ifndef IMX6ULL_SENSE_H
#define IMX6ULL_SENSE_H

#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#define SENSE_DEVICE_MAX 64
#define SENSE_ERROR_MAX 256
#define SENSE_EVENT_PATH_MAX 256

typedef struct {
    char device[SENSE_DEVICE_MAX];
    unsigned int width;
    unsigned int height;
    unsigned int fps_limit;
    unsigned int jpeg_quality;
    int http_port;
    char event_log[SENSE_EVENT_PATH_MAX];
    int motion_enabled;
    unsigned int motion_sample_fps;
    unsigned int motion_jpeg_scale_denom;
    unsigned int motion_pixel_delta_threshold;
    double motion_changed_ratio_threshold;
    unsigned int motion_cooldown_ms;
} SenseConfig;

typedef struct {
    int degraded;
    char device[SENSE_DEVICE_MAX];
    unsigned int width;
    unsigned int height;
    double fps;
    uint64_t frame_count;
    unsigned int client_count;
    int motion_enabled;
    int motion_state;
    double motion_score;
    double motion_sample_fps;
    uint64_t event_count;
    char last_error[SENSE_ERROR_MAX];
} StatusSnapshot;

typedef enum {
    JPEG_SNAPSHOT_TIMEOUT = 0,
    JPEG_SNAPSHOT_READY = 1,
    JPEG_SNAPSHOT_UNAVAILABLE = 2,
    JPEG_SNAPSHOT_STOPPED = -1,
    JPEG_SNAPSHOT_NO_MEMORY = -2,
    JPEG_SNAPSHOT_ERROR = -3
} JpegSnapshotStatus;

typedef struct {
    unsigned char *data;
    size_t size;
    size_t capacity;
    uint64_t sequence;
    uint64_t capture_generation;
} JpegSnapshot;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t frame_ready;
    pthread_cond_t clients_done;
    unsigned char *frame;
    size_t frame_size;
    size_t frame_capacity;
    uint64_t frame_sequence;
    uint64_t frame_generation;
    uint64_t capture_generation;
    uint64_t frame_count;
    uint64_t capture_base_count;
    uint64_t capture_started_ns;
    unsigned int worker_count;
    unsigned int client_count;
    unsigned int width;
    unsigned int height;
    double fps;
    int motion_enabled;
    int motion_state;
    double motion_score;
    double motion_sample_fps;
    uint64_t event_count;
    int degraded;
    int stop;
    char device[SENSE_DEVICE_MAX];
    char last_error[SENSE_ERROR_MAX];
} AppState;

typedef struct {
    const SenseConfig *config;
    AppState *state;
} CaptureContext;

typedef struct {
    const SenseConfig *config;
    AppState *state;
} MotionWorkerContext;

extern volatile sig_atomic_t g_signal_stop;

int config_load(const char *path, SenseConfig *config,
                char *error, size_t error_size);

int state_init(AppState *state);
void state_destroy(AppState *state);
void state_request_stop(AppState *state);
int state_should_stop(AppState *state);
void state_wait_for_clients(AppState *state);
void state_set_capture_active(AppState *state, const char *device,
                              unsigned int width, unsigned int height);
void state_set_degraded(AppState *state, const char *device,
                        const char *error);
int state_publish_frame(AppState *state, const void *frame, size_t frame_size);
int state_wait_frame(AppState *state, uint64_t *last_sequence,
                     unsigned char **buffer, size_t *capacity,
                     size_t *frame_size);
JpegSnapshotStatus state_wait_jpeg_snapshot(AppState *state,
                                            JpegSnapshot *snapshot);
void state_release_jpeg_snapshot(JpegSnapshot *snapshot);
void state_worker_delta(AppState *state, int delta);
void state_stream_client_delta(AppState *state, int delta);
void state_configure_motion(AppState *state, int enabled);
void state_update_motion(AppState *state, int motion_detected,
                         double score, double sample_fps,
                         uint64_t event_count);
void state_reset_motion_sample(AppState *state);
void state_snapshot(AppState *state, StatusSnapshot *snapshot);

void *capture_thread_main(void *argument);
void *motion_worker_thread_main(void *argument);
int http_server_run(AppState *state, int port);

#endif
