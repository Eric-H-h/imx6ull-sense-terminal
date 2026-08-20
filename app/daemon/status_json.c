#include "status_json.h"

#include <stdio.h>

static const char *camera_state_string(SenseCameraState state)
{
    switch (state) {
    case SENSE_CAMERA_INITIALIZING:
        return "initializing";
    case SENSE_CAMERA_ACTIVE:
        return "active";
    case SENSE_CAMERA_UNAVAILABLE:
        return "unavailable";
    default:
        return "unknown";
    }
}

static const char *event_log_state_string(SenseEventLogState state)
{
    switch (state) {
    case SENSE_EVENT_LOG_INITIALIZING:
        return "initializing";
    case SENSE_EVENT_LOG_DISABLED:
        return "disabled";
    case SENSE_EVENT_LOG_OK:
        return "ok";
    case SENSE_EVENT_LOG_UNAVAILABLE:
        return "unavailable";
    default:
        return "unknown";
    }
}

static void json_escape(const char *source, char *destination,
                        size_t destination_size)
{
    size_t output = 0U;

    if (destination_size == 0U) {
        return;
    }

    while ((*source != '\0') && (output + 1U < destination_size)) {
        unsigned char value = (unsigned char)*source++;

        if ((value == '"') || (value == '\\')) {
            if (output + 2U >= destination_size) {
                break;
            }
            destination[output++] = '\\';
            destination[output++] = (char)value;
        } else if (value >= 0x20U) {
            destination[output++] = (char)value;
        }
    }
    destination[output] = '\0';
}

int status_json_format(const StatusSnapshot *snapshot,
                       char *destination,
                       size_t capacity)
{
    char device[SENSE_DEVICE_MAX * 2U];
    char escaped_error[SENSE_ERROR_MAX * 2U];
    char error_json[(SENSE_ERROR_MAX * 2U) + 3U];
    char error_at_json[32];
    int length;

    if ((snapshot == NULL) || (destination == NULL) || (capacity == 0U)) {
        return -1;
    }

    json_escape(snapshot->device, device, sizeof(device));
    json_escape(snapshot->last_error, escaped_error, sizeof(escaped_error));
    if (snapshot->last_error[0] == '\0') {
        snprintf(error_json, sizeof(error_json), "null");
    } else {
        snprintf(error_json, sizeof(error_json), "\"%s\"", escaped_error);
    }
    if (snapshot->last_error_at_ms == 0U) {
        snprintf(error_at_json, sizeof(error_at_json), "null");
    } else {
        snprintf(error_at_json, sizeof(error_at_json), "%llu",
                 (unsigned long long)snapshot->last_error_at_ms);
    }

    length = snprintf(
        destination,
        capacity,
        "{\"ok\":%s,\"degraded\":%s,\"health\":\"%s\","
        "\"camera_state\":\"%s\",\"event_log_state\":\"%s\","
        "\"device\":\"%s\","
        "\"width\":%u,\"height\":%u,\"fps\":%.1f,"
        "\"frame_count\":%llu,\"client_count\":%u,"
        "\"motion_enabled\":%s,\"motion_state\":%s,"
        "\"motion_score\":%.6f,\"motion_sample_fps\":%.2f,"
        "\"event_count\":%llu,\"last_error\":%s,"
        "\"last_error_at\":%s,\"uptime\":%.3f}\n",
        snapshot->degraded ? "false" : "true",
        snapshot->degraded ? "true" : "false",
        snapshot->degraded ? "degraded" : "ok",
        camera_state_string(snapshot->camera_state),
        event_log_state_string(snapshot->event_log_state),
        device,
        snapshot->width,
        snapshot->height,
        snapshot->fps,
        (unsigned long long)snapshot->frame_count,
        snapshot->client_count,
        snapshot->motion_enabled ? "true" : "false",
        snapshot->motion_state ? "true" : "false",
        snapshot->motion_score,
        snapshot->motion_sample_fps,
        (unsigned long long)snapshot->event_count,
        error_json,
        error_at_json,
        snapshot->uptime_seconds);
    if ((length < 0) || ((size_t)length >= capacity)) {
        destination[0] = '\0';
        return -1;
    }
    return length;
}
