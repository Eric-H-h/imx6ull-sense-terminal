#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <stdint.h>

typedef enum {
    EVENT_LOG_OK = 0,
    EVENT_LOG_INVALID_ARGUMENT = -1,
    EVENT_LOG_FORMAT_ERROR = -2,
    EVENT_LOG_OPEN_ERROR = -3,
    EVENT_LOG_WRITE_ERROR = -4,
    EVENT_LOG_CLOSE_ERROR = -5
} EventLogStatus;

typedef struct {
    uint64_t timestamp_ms;
    uint64_t sequence;
    double score;
    double threshold;
    uint64_t changed_pixels;
    uint64_t total_pixels;
    uint64_t cooldown_ms;
} MotionEventRecord;

EventLogStatus event_log_append_motion(const char *path,
                                       const MotionEventRecord *event);

const char *event_log_status_string(EventLogStatus status);

#endif
