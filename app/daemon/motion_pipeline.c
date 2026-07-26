#include "motion_pipeline.h"

#include "event_log.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static int scale_is_valid(unsigned int scale_denom)
{
    return (scale_denom == 1U) || (scale_denom == 2U) ||
           (scale_denom == 4U) || (scale_denom == 8U);
}

static int config_is_valid(const MotionPipelineConfig *config)
{
    return (config != NULL) &&
           scale_is_valid(config->jpeg_scale_denom) &&
           (config->thresholds.pixel_delta_threshold >= 1U) &&
           (config->thresholds.pixel_delta_threshold <= 255U) &&
           isfinite(config->thresholds.changed_ratio_threshold) &&
           (config->thresholds.changed_ratio_threshold > 0.0) &&
           (config->thresholds.changed_ratio_threshold <= 1.0) &&
           (config->cooldown_ms > 0U) &&
           (config->event_log_path != NULL) &&
           (config->event_log_path[0] != '\0');
}

static int sample_is_valid(const MotionJpegSample *sample)
{
    return (sample != NULL) &&
           (sample->jpeg_data != NULL) &&
           (sample->jpeg_size > 0U);
}

static void copy_result(const MotionJpegSample *sample,
                        const MotionResult *motion,
                        const MotionEventDecision *decision,
                        MotionPipelineResult *result)
{
    result->sequence = sample->sequence;
    result->changed_pixels = motion->changed_pixels;
    result->total_pixels = motion->total_pixels;
    result->score = motion->changed_ratio;
    result->baseline_created = motion->baseline_created;
    result->motion_detected = motion->motion_detected;
    result->event_emitted = decision->event_emitted;
    result->phase = decision->phase;
    result->event_count = decision->event_count;
    result->cooldown_remaining_ms = decision->cooldown_remaining_ms;
}

static MotionEventRecord make_event_record(
    const MotionPipelineConfig *config,
    const MotionJpegSample *sample,
    const MotionPipelineResult *result)
{
    MotionEventRecord event = {
        .timestamp_ms = sample->realtime_ms,
        .sequence = sample->sequence,
        .score = result->score,
        .threshold = config->thresholds.changed_ratio_threshold,
        .changed_pixels = (uint64_t)result->changed_pixels,
        .total_pixels = (uint64_t)result->total_pixels,
        .cooldown_ms = config->cooldown_ms
    };

    return event;
}

MotionPipelineStatus motion_pipeline_process_jpeg(
    MotionPipeline *pipeline,
    const MotionPipelineConfig *config,
    const MotionJpegSample *sample,
    MotionPipelineResult *result)
{
    JpegGrayResult decode_status;
    MotionStatus detector_status;
    MotionEventGateStatus gate_status;
    MotionResult motion = {0};
    MotionEventDecision decision = {0};

    if (result != NULL) {
        memset(result, 0, sizeof(*result));
    }
    if ((pipeline == NULL) || (result == NULL) ||
        !config_is_valid(config) || !sample_is_valid(sample)) {
        return MOTION_PIPELINE_INVALID_ARGUMENT;
    }

    decode_status = jpeg_gray_decode(sample->jpeg_data,
                                     sample->jpeg_size,
                                     config->jpeg_scale_denom,
                                     &pipeline->gray_frame);
    if (decode_status != JPEG_GRAY_OK) {
        return MOTION_PIPELINE_DECODE_ERROR;
    }

    detector_status = motion_detector_process(
        &pipeline->detector,
        pipeline->gray_frame.pixels,
        pipeline->gray_frame.width,
        pipeline->gray_frame.height,
        pipeline->gray_frame.stride,
        config->thresholds,
        &motion);
    if (detector_status != MOTION_OK) {
        return MOTION_PIPELINE_DETECTOR_ERROR;
    }

    gate_status = motion_event_gate_update(&pipeline->event_gate,
                                           motion.motion_detected,
                                           sample->monotonic_ms,
                                           config->cooldown_ms,
                                           &decision);
    if (gate_status != MOTION_EVENT_GATE_OK) {
        return MOTION_PIPELINE_GATE_ERROR;
    }

    copy_result(sample, &motion, &decision, result);
    if (decision.event_emitted != 0) {
        MotionEventRecord event = make_event_record(config, sample, result);

        if (event_log_append_motion(config->event_log_path, &event) !=
            EVENT_LOG_OK) {
            return MOTION_PIPELINE_EVENT_LOG_ERROR;
        }
        result->event_logged = 1;
    }

    return MOTION_PIPELINE_OK;
}

void motion_pipeline_reset_baseline(MotionPipeline *pipeline)
{
    if (pipeline == NULL) {
        return;
    }

    motion_detector_reset(&pipeline->detector);
}

void motion_pipeline_destroy(MotionPipeline *pipeline)
{
    if (pipeline == NULL) {
        return;
    }

    jpeg_gray_frame_release(&pipeline->gray_frame);
    motion_detector_destroy(&pipeline->detector);
    memset(&pipeline->event_gate, 0, sizeof(pipeline->event_gate));
}

const char *motion_pipeline_status_string(MotionPipelineStatus status)
{
    switch (status) {
    case MOTION_PIPELINE_OK:
        return "ok";
    case MOTION_PIPELINE_INVALID_ARGUMENT:
        return "invalid argument";
    case MOTION_PIPELINE_DECODE_ERROR:
        return "jpeg decode error";
    case MOTION_PIPELINE_DETECTOR_ERROR:
        return "motion detector error";
    case MOTION_PIPELINE_GATE_ERROR:
        return "motion event gate error";
    case MOTION_PIPELINE_EVENT_LOG_ERROR:
        return "event log error";
    default:
        return "unknown motion pipeline status";
    }
}
