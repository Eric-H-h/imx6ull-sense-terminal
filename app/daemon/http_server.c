#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct {
    int fd;
    AppState *state;
} ClientContext;

static const char index_html[] =
    "<!doctype html><html><head><meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>i.MX6ULL Sense Terminal</title>"
    "<style>html,body{margin:0;width:100%;height:100%;background:#111;color:#eee;}"
    "body{display:grid;place-items:center;font-family:sans-serif;}"
    "main{width:min(100%,960px);padding:16px;box-sizing:border-box;}"
    "h1{font-size:18px;font-weight:600;margin:0 0 10px;}"
    "img{display:block;width:100%;height:auto;background:#000;border:1px solid #333;}"
    "</style></head><body><main><h1>i.MX6ULL Sense Terminal</h1>"
    "<img src=\"/stream\" alt=\"Camera stream\"></main></body></html>";

static int send_all(int fd, const void *data, size_t length)
{
    const unsigned char *cursor = data;

    while (length > 0) {
        ssize_t sent = send(fd, cursor, length, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (sent == 0) {
            return -1;
        }
        cursor += (size_t)sent;
        length -= (size_t)sent;
    }
    return 0;
}

static int send_response(int fd, const char *status, const char *content_type,
                         const char *body, size_t body_size)
{
    char header[512];
    int header_size = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n\r\n",
        status, content_type, body_size);

    if ((header_size < 0) || ((size_t)header_size >= sizeof(header))) {
        return -1;
    }
    if (send_all(fd, header, (size_t)header_size) < 0) {
        return -1;
    }
    return send_all(fd, body, body_size);
}

static void json_escape(const char *source, char *destination,
                        size_t destination_size)
{
    size_t output = 0;

    if (destination_size == 0) {
        return;
    }

    while ((*source != '\0') && (output + 1 < destination_size)) {
        unsigned char value = (unsigned char)*source++;

        if ((value == '"') || (value == '\\')) {
            if (output + 2 >= destination_size) {
                break;
            }
            destination[output++] = '\\';
            destination[output++] = (char)value;
        } else if (value >= 0x20) {
            destination[output++] = (char)value;
        }
    }
    destination[output] = '\0';
}

static void handle_status(int fd, AppState *state)
{
    StatusSnapshot snapshot;
    char device[SENSE_DEVICE_MAX * 2];
    char escaped_error[SENSE_ERROR_MAX * 2];
    char error_json[(SENSE_ERROR_MAX * 2) + 3];
    char body[1024];
    int body_size;

    state_snapshot(state, &snapshot);
    json_escape(snapshot.device, device, sizeof(device));
    json_escape(snapshot.last_error, escaped_error, sizeof(escaped_error));

    if (snapshot.last_error[0] == '\0') {
        snprintf(error_json, sizeof(error_json), "null");
    } else {
        snprintf(error_json, sizeof(error_json), "\"%s\"", escaped_error);
    }

    body_size = snprintf(body, sizeof(body),
        "{\"ok\":%s,\"degraded\":%s,\"device\":\"%s\","
        "\"width\":%u,\"height\":%u,\"fps\":%.1f,"
        "\"frame_count\":%llu,\"client_count\":%u,"
        "\"motion_state\":false,\"event_count\":0,"
        "\"last_error\":%s}\n",
        snapshot.degraded ? "false" : "true",
        snapshot.degraded ? "true" : "false",
        device, snapshot.width, snapshot.height, snapshot.fps,
        (unsigned long long)snapshot.frame_count, snapshot.client_count,
        error_json);

    if ((body_size > 0) && ((size_t)body_size < sizeof(body))) {
        send_response(fd, "200 OK", "application/json", body,
                      (size_t)body_size);
    }
}

static void handle_stream(int fd, AppState *state)
{
    static const char stream_header[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-store, no-cache, must-revalidate\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n\r\n";
    unsigned char *frame = NULL;
    size_t capacity = 0;
    size_t frame_size = 0;
    uint64_t sequence = 0;

    if (send_all(fd, stream_header, sizeof(stream_header) - 1) < 0) {
        return;
    }

    state_stream_client_delta(state, 1);
    while (!state_should_stop(state)) {
        char part_header[160];
        int part_header_size;
        int wait_result = state_wait_frame(state, &sequence, &frame,
                                           &capacity, &frame_size);

        if (wait_result == 0) {
            continue;
        }
        if (wait_result < 0) {
            break;
        }

        part_header_size = snprintf(part_header, sizeof(part_header),
            "--frame\r\nContent-Type: image/jpeg\r\n"
            "Content-Length: %zu\r\n\r\n", frame_size);
        if ((part_header_size < 0) ||
            ((size_t)part_header_size >= sizeof(part_header)) ||
            (send_all(fd, part_header, (size_t)part_header_size) < 0) ||
            (send_all(fd, frame, frame_size) < 0) ||
            (send_all(fd, "\r\n", 2) < 0)) {
            break;
        }
    }

    free(frame);
    state_stream_client_delta(state, -1);
}

static void *client_thread_main(void *argument)
{
    ClientContext *context = argument;
    char request[1024];
    char method[8];
    char path[128];
    ssize_t bytes;

    bytes = recv(context->fd, request, sizeof(request) - 1, 0);
    if (bytes > 0) {
        request[bytes] = '\0';
        if (sscanf(request, "%7s %127s", method, path) == 2) {
            if (strcmp(method, "GET") != 0) {
                static const char body[] = "method not allowed\n";
                send_response(context->fd, "405 Method Not Allowed",
                              "text/plain", body, sizeof(body) - 1);
            } else if (strcmp(path, "/") == 0) {
                send_response(context->fd, "200 OK", "text/html; charset=utf-8",
                              index_html, sizeof(index_html) - 1);
            } else if (strcmp(path, "/status") == 0) {
                handle_status(context->fd, context->state);
            } else if (strcmp(path, "/stream") == 0) {
                handle_stream(context->fd, context->state);
            } else {
                static const char body[] = "not found\n";
                send_response(context->fd, "404 Not Found", "text/plain",
                              body, sizeof(body) - 1);
            }
        }
    }

    close(context->fd);
    state_worker_delta(context->state, -1);
    free(context);
    return NULL;
}

int http_server_run(AppState *state, int port)
{
    struct sockaddr_in address;
    int reuse = 1;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd < 0) {
        fprintf(stderr, "HTTP socket failed: %s\n", strerror(errno));
        return -1;
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        fprintf(stderr, "HTTP bind on port %d failed: %s\n", port,
                strerror(errno));
        close(listen_fd);
        return -1;
    }
    if (listen(listen_fd, 8) < 0) {
        fprintf(stderr, "HTTP listen failed: %s\n", strerror(errno));
        close(listen_fd);
        return -1;
    }

    while (!state_should_stop(state)) {
        struct pollfd poll_descriptor = {listen_fd, POLLIN, 0};
        int poll_result = poll(&poll_descriptor, 1, 500);

        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "HTTP poll failed: %s\n", strerror(errno));
            close(listen_fd);
            return -1;
        }
        if (poll_result == 0) {
            continue;
        }
        if ((poll_descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            fprintf(stderr, "HTTP listening socket failed\n");
            close(listen_fd);
            return -1;
        }
        if ((poll_descriptor.revents & POLLIN) != 0) {
            struct timeval io_timeout = {3, 0};
            ClientContext *context = malloc(sizeof(*context));
            pthread_t thread;

            if (context == NULL) {
                continue;
            }
            context->fd = accept(listen_fd, NULL, NULL);
            context->state = state;
            if (context->fd < 0) {
                free(context);
                if (errno == EINTR) {
                    continue;
                }
                continue;
            }

            if ((setsockopt(context->fd, SOL_SOCKET, SO_RCVTIMEO,
                            &io_timeout, sizeof(io_timeout)) < 0) ||
                (setsockopt(context->fd, SOL_SOCKET, SO_SNDTIMEO,
                            &io_timeout, sizeof(io_timeout)) < 0)) {
                fprintf(stderr, "HTTP client timeout setup failed: %s\n",
                        strerror(errno));
                close(context->fd);
                free(context);
                continue;
            }
            state_worker_delta(state, 1);
            if (pthread_create(&thread, NULL, client_thread_main, context) != 0) {
                state_worker_delta(state, -1);
                close(context->fd);
                free(context);
                continue;
            }
            pthread_detach(thread);
        }
    }

    close(listen_fd);
    return 0;
}
