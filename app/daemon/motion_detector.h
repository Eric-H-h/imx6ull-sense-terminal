#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include <stddef.h>

typedef enum {
    MOTION_OK = 0,
    MOTION_INVALID_ARGUMENT = -1,
    MOTION_NO_MEMORY = -2,
    MOTION_SIZE_OVERFLOW = -3
} MotionStatus;

typedef struct {
    unsigned int pixel_delta_threshold;
    double changed_ratio_threshold;
} MotionThresholds;

typedef struct {
    size_t total_pixels;
    size_t changed_pixels;
    double changed_ratio;
    int motion_detected;
    int baseline_created;
} MotionResult;

typedef struct {
    unsigned char *previous_pixels;
    size_t capacity;
    unsigned int width;
    unsigned int height;
    size_t stride;
    int baseline_ready;
} MotionDetector;

MotionStatus motion_detector_process(MotionDetector *detector,
                                     const unsigned char *pixels,
                                     unsigned int width,
                                     unsigned int height,
                                     size_t stride,
                                     MotionThresholds thresholds,
                                     MotionResult *result);

/* Keep the allocated buffer, but make the next frame establish a baseline. */
void motion_detector_reset(MotionDetector *detector);

/* Release all detector-owned memory. Safe to call with NULL. */
void motion_detector_destroy(MotionDetector *detector);

const char *motion_status_string(MotionStatus status);

#endif
