#define _POSIX_C_SOURCE 200809L

#include "event_log.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

enum {
    EVENT_LOG_LINE_CAPACITY = 512
};

static int event_is_valid(const MotionEventRecord *event)
{
    return (event != NULL) &&
           isfinite(event->score) &&
           (event->score >= 0.0) &&
           (event->score <= 1.0) &&
           isfinite(event->threshold) &&
           (event->threshold > 0.0) &&
           (event->threshold <= 1.0) &&
           (event->total_pixels > 0U) &&
           (event->changed_pixels <= event->total_pixels) &&
           (event->cooldown_ms > 0U);
}

static EventLogStatus format_motion_event(const MotionEventRecord *event,
                                          char *line,
                                          size_t capacity,
                                          size_t *line_length)
{
    int length;

    length = snprintf(
        line,
        capacity,
        "{\"ts_ms\":%" PRIu64
        ",\"type\":\"motion\""
        ",\"sequence\":%" PRIu64
        ",\"score\":%.6f"
        ",\"threshold\":%.6f"
        ",\"changed_pixels\":%" PRIu64
        ",\"total_pixels\":%" PRIu64
        ",\"cooldown_ms\":%" PRIu64 "}\n",
        event->timestamp_ms,
        event->sequence,
        event->score,
        event->threshold,
        event->changed_pixels,
        event->total_pixels,
        event->cooldown_ms);
    if ((length < 0) || ((size_t)length >= capacity)) {
        errno = EOVERFLOW;
        return EVENT_LOG_FORMAT_ERROR;
    }

    *line_length = (size_t)length;
    return EVENT_LOG_OK;
}

static EventLogStatus write_all(int file_descriptor,
                                const char *data,
                                size_t length)
{
    size_t written = 0U;

    while (written < length) {
        ssize_t result = write(file_descriptor,
                               data + written,
                               length - written);

        if (result > 0) {
            written += (size_t)result;
            continue;
        }
        if ((result < 0) && (errno == EINTR)) {
            continue;
        }
        if (result == 0) {
            errno = EIO;
        }
        return EVENT_LOG_WRITE_ERROR;
    }

    return EVENT_LOG_OK;
}

EventLogStatus event_log_append_motion(const char *path,
                                       const MotionEventRecord *event)
{
    char line[EVENT_LOG_LINE_CAPACITY];
    size_t line_length = 0U;
    int file_descriptor;
    int saved_errno;
    EventLogStatus status;

    if ((path == NULL) || (path[0] == '\0') || !event_is_valid(event)) {
        errno = EINVAL;
        return EVENT_LOG_INVALID_ARGUMENT;
    }

    status = format_motion_event(event, line, sizeof(line), &line_length);
    if (status != EVENT_LOG_OK) {
        return status;
    }

    file_descriptor = open(path,
                           O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC,
                           0644);
    if (file_descriptor < 0) {
        return EVENT_LOG_OPEN_ERROR;
    }

    status = write_all(file_descriptor, line, line_length);
    if (status != EVENT_LOG_OK) {
        saved_errno = errno;
        (void)close(file_descriptor);
        errno = saved_errno;
        return status;
    }

    if (close(file_descriptor) != 0) {
        return EVENT_LOG_CLOSE_ERROR;
    }
    return EVENT_LOG_OK;
}

const char *event_log_status_string(EventLogStatus status)
{
    switch (status) {
    case EVENT_LOG_OK:
        return "ok";
    case EVENT_LOG_INVALID_ARGUMENT:
        return "invalid argument";
    case EVENT_LOG_FORMAT_ERROR:
        return "format error";
    case EVENT_LOG_OPEN_ERROR:
        return "open error";
    case EVENT_LOG_WRITE_ERROR:
        return "write error";
    case EVENT_LOG_CLOSE_ERROR:
        return "close error";
    default:
        return "unknown event log status";
    }
}
