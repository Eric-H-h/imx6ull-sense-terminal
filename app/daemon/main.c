#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

volatile sig_atomic_t g_signal_stop = 0;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    g_signal_stop = 1;
}

static int install_signal_handlers(void)
{
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);

    if ((sigaction(SIGINT, &action, NULL) < 0) ||
        (sigaction(SIGTERM, &action, NULL) < 0)) {
        return -1;
    }

    signal(SIGPIPE, SIG_IGN);
    return 0;
}

static void print_usage(const char *program)
{
    fprintf(stderr, "usage: %s [-c config.json]\n", program);
}

int main(int argc, char **argv)
{
    const char *config_path = "config/config.json";
    SenseConfig config;
    AppState state;
    CaptureContext capture_context;
    MotionWorkerContext motion_context;
    pthread_t capture_thread;
    pthread_t motion_thread;
    char error[256];
    int http_result;

    setvbuf(stdout, NULL, _IOLBF, 0);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            if (++i >= argc) {
                print_usage(argv[0]);
                return 2;
            }
            config_path = argv[i];
        } else if ((strcmp(argv[i], "-h") == 0) ||
                   (strcmp(argv[i], "--help") == 0)) {
            print_usage(argv[0]);
            return 0;
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }

    if (config_load(config_path, &config, error, sizeof(error)) < 0) {
        fprintf(stderr, "config error: %s\n", error);
        return 2;
    }

    if (install_signal_handlers() < 0) {
        fprintf(stderr, "signal setup failed: %s\n", strerror(errno));
        return 1;
    }

    if (state_init(&state) < 0) {
        fprintf(stderr, "state initialization failed\n");
        return 1;
    }
    state_configure_motion(&state, config.motion_enabled);

    capture_context.config = &config;
    capture_context.state = &state;
    motion_context.config = &config;
    motion_context.state = &state;

    if (pthread_create(&capture_thread, NULL, capture_thread_main,
                       &capture_context) != 0) {
        fprintf(stderr, "capture thread creation failed\n");
        state_destroy(&state);
        return 1;
    }

    if (pthread_create(&motion_thread, NULL, motion_worker_thread_main,
                       &motion_context) != 0) {
        fprintf(stderr, "motion worker thread creation failed\n");
        state_request_stop(&state);
        pthread_join(capture_thread, NULL);
        state_destroy(&state);
        return 1;
    }

    printf("imx6ull-sense listening on 0.0.0.0:%d\n", config.http_port);
    printf("camera selector: %s, request: MJPG %ux%u@%u\n",
           config.device, config.width, config.height, config.fps_limit);

    http_result = http_server_run(&state, config.http_port);
    state_request_stop(&state);
    pthread_join(capture_thread, NULL);
    pthread_join(motion_thread, NULL);
    state_wait_for_clients(&state);
    state_destroy(&state);

    if ((http_result < 0) && !g_signal_stop) {
        return 1;
    }
    return 0;
}

