#include "motion_detector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static MotionStatus validate_frame_layout(unsigned int width,
                                          unsigned int height,
                                          size_t stride,
                                          size_t *pixel_count)
{
    size_t rows_before_last;

    if ((width == 0U) || (height == 0U) || (stride < (size_t)width)) {
        return MOTION_INVALID_ARGUMENT;
    }
    if ((size_t)width > (SIZE_MAX / (size_t)height)) {
        return MOTION_SIZE_OVERFLOW;
    }

    rows_before_last = (size_t)height - 1U;
    if ((rows_before_last != 0U) &&
        (stride > ((SIZE_MAX - (size_t)width) / rows_before_last))) {
        return MOTION_SIZE_OVERFLOW;
    }

    *pixel_count = (size_t)width * (size_t)height;
    return MOTION_OK;
}

static int thresholds_are_valid(MotionThresholds thresholds)
{
    return (thresholds.pixel_delta_threshold >= 1U) &&
           (thresholds.pixel_delta_threshold <= 255U) &&
           (thresholds.changed_ratio_threshold > 0.0) &&
           (thresholds.changed_ratio_threshold <= 1.0);
}

static MotionStatus ensure_capacity(MotionDetector *detector,
                                    size_t pixel_count)
{
    unsigned char *resized;

    if (detector->capacity >= pixel_count) {
        return MOTION_OK;
    }

    resized = realloc(detector->previous_pixels, pixel_count);
    if (resized == NULL) {
        return MOTION_NO_MEMORY;
    }

    detector->previous_pixels = resized;
    detector->capacity = pixel_count;
    return MOTION_OK;
}

static void copy_frame_to_baseline(MotionDetector *detector,
                                   const unsigned char *pixels,
                                   unsigned int width,
                                   unsigned int height,
                                   size_t stride)
{
    for (unsigned int row = 0U; row < height; ++row) {
        memcpy(detector->previous_pixels + ((size_t)row * width),
               pixels + ((size_t)row * stride),
               width);
    }

    detector->width = width;
    detector->height = height;
    detector->stride = width;
    detector->baseline_ready = 1;
}

MotionStatus motion_detector_process(MotionDetector *detector,
                                     const unsigned char *pixels,
                                     unsigned int width,
                                     unsigned int height,
                                     size_t stride,
                                     MotionThresholds thresholds,
                                     MotionResult *result)
{
    size_t pixel_count;
    size_t changed_pixels = 0U;
    MotionStatus status;

    if (result != NULL) {
        memset(result, 0, sizeof(*result));
    }
    if ((detector == NULL) || (pixels == NULL) || (result == NULL) ||
        !thresholds_are_valid(thresholds)) {
        return MOTION_INVALID_ARGUMENT;
    }

    status = validate_frame_layout(width, height, stride, &pixel_count);
    if (status != MOTION_OK) {
        return status;
    }

    status = ensure_capacity(detector, pixel_count);
    if (status != MOTION_OK) {
        return status;
    }

    result->total_pixels = pixel_count;
    if (!detector->baseline_ready || (detector->width != width) ||
        (detector->height != height)) {
        copy_frame_to_baseline(detector, pixels, width, height, stride);
        result->baseline_created = 1;
        return MOTION_OK;
    }

    for (unsigned int row = 0U; row < height; ++row) {
        const unsigned char *current_row = pixels + ((size_t)row * stride);
        unsigned char *previous_row =
            detector->previous_pixels + ((size_t)row * width);

        for (unsigned int column = 0U; column < width; ++column) {
            unsigned int current = current_row[column];
            unsigned int previous = previous_row[column];
            unsigned int difference = (current >= previous)
                                          ? (current - previous)
                                          : (previous - current);

            if (difference >= thresholds.pixel_delta_threshold) {
                ++changed_pixels;
            }
        }
        memcpy(previous_row, current_row, width);
    }

    result->changed_pixels = changed_pixels;
    result->changed_ratio = (double)changed_pixels / (double)pixel_count;
    result->motion_detected =
        result->changed_ratio >= thresholds.changed_ratio_threshold;
    return MOTION_OK;
}

void motion_detector_reset(MotionDetector *detector)
{
    if (detector == NULL) {
        return;
    }

    detector->width = 0U;
    detector->height = 0U;
    detector->stride = 0U;
    detector->baseline_ready = 0;
}

void motion_detector_destroy(MotionDetector *detector)
{
    if (detector == NULL) {
        return;
    }

    free(detector->previous_pixels);
    memset(detector, 0, sizeof(*detector));
}

const char *motion_status_string(MotionStatus status)
{
    switch (status) {
    case MOTION_OK:
        return "ok";
    case MOTION_INVALID_ARGUMENT:
        return "invalid argument";
    case MOTION_NO_MEMORY:
        return "out of memory";
    case MOTION_SIZE_OVERFLOW:
        return "frame size overflow";
    default:
        return "unknown motion status";
    }
}
