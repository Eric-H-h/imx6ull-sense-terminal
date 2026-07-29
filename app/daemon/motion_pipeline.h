#ifndef MOTION_PIPELINE_H
#define MOTION_PIPELINE_H

#include "jpeg_decoder.h"
#include "motion_detector.h"
#include "motion_event_gate.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MOTION_PIPELINE_OK = 0,
    MOTION_PIPELINE_INVALID_ARGUMENT = -1,
    MOTION_PIPELINE_DECODE_ERROR = -2,
    MOTION_PIPELINE_DETECTOR_ERROR = -3,
    MOTION_PIPELINE_GATE_ERROR = -4,
    MOTION_PIPELINE_EVENT_LOG_ERROR = -5
} MotionPipelineStatus;

typedef struct {
    unsigned int jpeg_scale_denom;
    MotionThresholds thresholds;
    uint64_t cooldown_ms;
    const char *event_log_path;
} MotionPipelineConfig;

typedef struct {
    const unsigned char *jpeg_data;
    size_t jpeg_size;
    uint64_t sequence;
    uint64_t monotonic_ms;
    uint64_t realtime_ms;
} MotionJpegSample;

typedef struct {
    uint64_t sequence;
    size_t changed_pixels;
    size_t total_pixels;
    double score;
    int baseline_created;
    int motion_detected;
    int event_emitted;
    int event_logged;
    MotionEventPhase phase;
    uint64_t event_count;
    uint64_t cooldown_remaining_ms;
} MotionPipelineResult;

typedef struct {
    JpegGrayFrame gray_frame;
    MotionDetector detector;
    MotionEventGate event_gate;
} MotionPipeline;

/*
 * The caller zero-initializes pipeline before first use. Config and sample are
 * read only during the call; the pipeline does not retain their pointers.
 */
MotionPipelineStatus motion_pipeline_process_jpeg(
    MotionPipeline *pipeline,
    const MotionPipelineConfig *config,
    const MotionJpegSample *sample,
    MotionPipelineResult *result);

/* Rebuild the detector baseline without clearing cooldown or event count. */
void motion_pipeline_reset_baseline(MotionPipeline *pipeline);

void motion_pipeline_destroy(MotionPipeline *pipeline);

const char *motion_pipeline_status_string(MotionPipelineStatus status);

#endif
