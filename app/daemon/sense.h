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
    unsigned int motion_threshold;
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
    char last_error[SENSE_ERROR_MAX];
} StatusSnapshot;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t frame_ready;
    pthread_cond_t clients_done;
    unsigned char *frame;
    size_t frame_size;
    size_t frame_capacity;
    uint64_t frame_sequence;
    uint64_t frame_count;
    uint64_t capture_base_count;
    uint64_t capture_started_ns;
    unsigned int worker_count;
    unsigned int client_count;
    unsigned int width;
    unsigned int height;
    double fps;
    int degraded;
    int stop;
    char device[SENSE_DEVICE_MAX];
    char last_error[SENSE_ERROR_MAX];
} AppState;

typedef struct {
    const SenseConfig *config;
    AppState *state;
} CaptureContext;

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
void state_worker_delta(AppState *state, int delta);
void state_stream_client_delta(AppState *state, int delta);
void state_snapshot(AppState *state, StatusSnapshot *snapshot);

void *capture_thread_main(void *argument);
int http_server_run(AppState *state, int port);

#endif
