#include "motion_pipeline.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <jpeglib.h>

static int failure_count = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                             \
            ++failure_count;                                                 \
        }                                                                    \
    } while (0)

static void create_test_jpeg(int changed,
                             unsigned char **jpeg_data,
                             unsigned long *jpeg_size)
{
    struct jpeg_compress_struct encoder;
    struct jpeg_error_mgr error;
    unsigned char row[64];

    encoder.err = jpeg_std_error(&error);
    jpeg_create_compress(&encoder);
    jpeg_mem_dest(&encoder, jpeg_data, jpeg_size);

    encoder.image_width = 64U;
    encoder.image_height = 64U;
    encoder.input_components = 1;
    encoder.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&encoder);
    jpeg_set_quality(&encoder, 100, TRUE);
    jpeg_start_compress(&encoder, TRUE);

    while (encoder.next_scanline < encoder.image_height) {
        JSAMPROW row_pointer = row;

        for (unsigned int column = 0U; column < 64U; ++column) {
            row[column] = (changed && (column < 32U)) ? 220U : 32U;
        }
        jpeg_write_scanlines(&encoder, &row_pointer, 1U);
    }

    jpeg_finish_compress(&encoder);
    jpeg_destroy_compress(&encoder);
}

static MotionPipelineConfig make_config(const char *event_log_path)
{
    MotionPipelineConfig config = {
        .jpeg_scale_denom = 4U,
        .thresholds = {
            .pixel_delta_threshold = 20U,
            .changed_ratio_threshold = 0.10
        },
        .cooldown_ms = 3000U,
        .event_log_path = event_log_path
    };

    return config;
}

static MotionPipelineStatus process_sample(MotionPipeline *pipeline,
                                           const MotionPipelineConfig *config,
                                           const unsigned char *jpeg_data,
                                           size_t jpeg_size,
                                           uint64_t sequence,
                                           uint64_t monotonic_ms,
                                           MotionPipelineResult *result)
{
    MotionJpegSample sample = {
        .jpeg_data = jpeg_data,
        .jpeg_size = jpeg_size,
        .sequence = sequence,
        .monotonic_ms = monotonic_ms,
        .realtime_ms = UINT64_C(1784973600000) + monotonic_ms
    };

    return motion_pipeline_process_jpeg(pipeline, config, &sample, result);
}

static void make_test_path(char *path, size_t capacity, const char *suffix)
{
    int length = snprintf(path,
                          capacity,
                          "/tmp/imx6ull-motion-pipeline-%ld-%s.jsonl",
                          (long)getpid(),
                          suffix);

    CHECK((length > 0) && ((size_t)length < capacity));
}

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *contents;

    if (file == NULL) {
        return NULL;
    }
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0)) {
        fclose(file);
        return NULL;
    }

    contents = malloc((size_t)length + 1U);
    if (contents == NULL) {
        fclose(file);
        return NULL;
    }
    if (fread(contents, 1U, (size_t)length, file) != (size_t)length) {
        free(contents);
        fclose(file);
        return NULL;
    }

    contents[length] = '\0';
    fclose(file);
    return contents;
}

static size_t count_newlines(const char *text)
{
    size_t count = 0U;

    while (*text != '\0') {
        if (*text == '\n') {
            ++count;
        }
        ++text;
    }
    return count;
}

static void test_full_pipeline_and_cooldown(void)
{
    unsigned char *base_jpeg = NULL;
    unsigned char *changed_jpeg = NULL;
    unsigned long base_size = 0U;
    unsigned long changed_size = 0U;
    char path[192];
    char *contents;
    MotionPipeline pipeline = {0};
    MotionPipelineResult result;
    MotionPipelineConfig config;

    create_test_jpeg(0, &base_jpeg, &base_size);
    create_test_jpeg(1, &changed_jpeg, &changed_size);
    CHECK((base_jpeg != NULL) && (changed_jpeg != NULL));
    if ((base_jpeg == NULL) || (changed_jpeg == NULL)) {
        free(base_jpeg);
        free(changed_jpeg);
        return;
    }

    make_test_path(path, sizeof(path), "full");
    (void)unlink(path);
    config = make_config(path);

    CHECK(process_sample(&pipeline, &config, base_jpeg, base_size,
                         1U, 0U, &result) == MOTION_PIPELINE_OK);
    CHECK(result.baseline_created == 1);
    CHECK(result.motion_detected == 0);
    CHECK(result.event_emitted == 0);
    CHECK(access(path, F_OK) != 0);

    CHECK(process_sample(&pipeline, &config, base_jpeg, base_size,
                         2U, 1000U, &result) == MOTION_PIPELINE_OK);
    CHECK(result.score == 0.0);
    CHECK(result.motion_detected == 0);

    CHECK(process_sample(&pipeline, &config, changed_jpeg, changed_size,
                         3U, 3000U, &result) == MOTION_PIPELINE_OK);
    CHECK(result.score > 0.40);
    CHECK(result.motion_detected == 1);
    CHECK(result.event_emitted == 1);
    CHECK(result.event_logged == 1);
    CHECK(result.event_count == 1U);

    CHECK(process_sample(&pipeline, &config, base_jpeg, base_size,
                         4U, 4000U, &result) == MOTION_PIPELINE_OK);
    CHECK(result.motion_detected == 1);
    CHECK(result.event_emitted == 0);
    CHECK(result.event_count == 1U);

    CHECK(process_sample(&pipeline, &config, changed_jpeg, changed_size,
                         5U, 6000U, &result) == MOTION_PIPELINE_OK);
    CHECK(result.motion_detected == 1);
    CHECK(result.event_emitted == 1);
    CHECK(result.event_logged == 1);
    CHECK(result.event_count == 2U);

    contents = read_file(path);
    CHECK(contents != NULL);
    if (contents != NULL) {
        CHECK(count_newlines(contents) == 2U);
        CHECK(strstr(contents, "\"sequence\":3") != NULL);
        CHECK(strstr(contents, "\"sequence\":5") != NULL);
        free(contents);
    }

    motion_pipeline_destroy(&pipeline);
    free(base_jpeg);
    free(changed_jpeg);
    (void)unlink(path);
}

static void test_baseline_reset_does_not_emit(void)
{
    unsigned char *base_jpeg = NULL;
    unsigned char *changed_jpeg = NULL;
    unsigned long base_size = 0U;
    unsigned long changed_size = 0U;
    char path[192];
    MotionPipeline pipeline = {0};
    MotionPipelineResult result;
    MotionPipelineConfig config;

    create_test_jpeg(0, &base_jpeg, &base_size);
    create_test_jpeg(1, &changed_jpeg, &changed_size);
    make_test_path(path, sizeof(path), "reset");
    (void)unlink(path);
    config = make_config(path);

    CHECK(process_sample(&pipeline, &config, base_jpeg, base_size,
                         1U, 0U, &result) == MOTION_PIPELINE_OK);
    motion_pipeline_reset_baseline(&pipeline);
    CHECK(process_sample(&pipeline, &config, changed_jpeg, changed_size,
                         2U, 1000U, &result) == MOTION_PIPELINE_OK);
    CHECK(result.baseline_created == 1);
    CHECK(result.motion_detected == 0);
    CHECK(result.event_emitted == 0);
    CHECK(access(path, F_OK) != 0);

    motion_pipeline_destroy(&pipeline);
    motion_pipeline_destroy(&pipeline);
    motion_pipeline_destroy(NULL);
    motion_pipeline_reset_baseline(NULL);
    free(base_jpeg);
    free(changed_jpeg);
}

static void test_invalid_jpeg_does_not_change_event_state(void)
{
    static const unsigned char invalid_jpeg[] = {
        0x6eU, 0x6fU, 0x74U, 0x2dU, 0x6aU, 0x70U, 0x65U, 0x67U
    };
    char path[192];
    MotionPipeline pipeline = {0};
    MotionPipelineResult result;
    MotionPipelineConfig config;

    make_test_path(path, sizeof(path), "invalid-jpeg");
    (void)unlink(path);
    config = make_config(path);

    CHECK(process_sample(&pipeline, &config,
                         invalid_jpeg, sizeof(invalid_jpeg),
                         1U, 0U, &result) ==
          MOTION_PIPELINE_DECODE_ERROR);
    CHECK(pipeline.event_gate.event_count == 0U);
    CHECK(access(path, F_OK) != 0);

    motion_pipeline_destroy(&pipeline);
}

static void test_invalid_arguments_and_config(void)
{
    static const unsigned char jpeg[] = {0xffU, 0xd8U, 0xffU, 0xd9U};
    MotionPipeline pipeline = {0};
    MotionPipelineResult result;
    MotionPipelineConfig config = make_config("/tmp/unused.jsonl");
    MotionJpegSample sample = {
        .jpeg_data = jpeg,
        .jpeg_size = sizeof(jpeg)
    };

    CHECK(motion_pipeline_process_jpeg(NULL, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    CHECK(motion_pipeline_process_jpeg(&pipeline, NULL, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, NULL, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, NULL) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);

    config.jpeg_scale_denom = 3U;
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    config = make_config("/tmp/unused.jsonl");
    config.thresholds.pixel_delta_threshold = 0U;
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    config = make_config("/tmp/unused.jsonl");
    config.thresholds.changed_ratio_threshold = NAN;
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    config = make_config("/tmp/unused.jsonl");
    config.cooldown_ms = 0U;
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    config = make_config("");
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);

    config = make_config("/tmp/unused.jsonl");
    sample.jpeg_data = NULL;
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);
    sample.jpeg_data = jpeg;
    sample.jpeg_size = 0U;
    CHECK(motion_pipeline_process_jpeg(&pipeline, &config, &sample, &result) ==
          MOTION_PIPELINE_INVALID_ARGUMENT);

    motion_pipeline_destroy(&pipeline);
}

static void test_event_log_failure_is_reported(void)
{
    unsigned char *base_jpeg = NULL;
    unsigned char *changed_jpeg = NULL;
    unsigned long base_size = 0U;
    unsigned long changed_size = 0U;
    MotionPipeline pipeline = {0};
    MotionPipelineResult result;
    MotionPipelineConfig config = make_config("/dev/full");

    if (access("/dev/full", W_OK) != 0) {
        return;
    }

    create_test_jpeg(0, &base_jpeg, &base_size);
    create_test_jpeg(1, &changed_jpeg, &changed_size);

    CHECK(process_sample(&pipeline, &config, base_jpeg, base_size,
                         1U, 0U, &result) == MOTION_PIPELINE_OK);
    CHECK(process_sample(&pipeline, &config, changed_jpeg, changed_size,
                         2U, 1000U, &result) ==
          MOTION_PIPELINE_EVENT_LOG_ERROR);
    CHECK(result.event_emitted == 1);
    CHECK(result.event_logged == 0);
    CHECK(result.event_count == 1U);
    CHECK(errno == ENOSPC);

    motion_pipeline_destroy(&pipeline);
    free(base_jpeg);
    free(changed_jpeg);
}

static void test_status_strings(void)
{
    CHECK(motion_pipeline_status_string(MOTION_PIPELINE_OK) != NULL);
    CHECK(motion_pipeline_status_string(
              MOTION_PIPELINE_INVALID_ARGUMENT) != NULL);
    CHECK(motion_pipeline_status_string(
              MOTION_PIPELINE_DECODE_ERROR) != NULL);
    CHECK(motion_pipeline_status_string(
              MOTION_PIPELINE_DETECTOR_ERROR) != NULL);
    CHECK(motion_pipeline_status_string(
              MOTION_PIPELINE_GATE_ERROR) != NULL);
    CHECK(motion_pipeline_status_string(
              MOTION_PIPELINE_EVENT_LOG_ERROR) != NULL);
    CHECK(motion_pipeline_status_string((MotionPipelineStatus)-99) != NULL);
}

int main(void)
{
    test_full_pipeline_and_cooldown();
    test_baseline_reset_does_not_emit();
    test_invalid_jpeg_does_not_change_event_state();
    test_invalid_arguments_and_config();
    test_event_log_failure_is_reported();
    test_status_strings();

    if (failure_count != 0) {
        fprintf(stderr, "motion pipeline tests: %d failure(s)\n",
                failure_count);
        return 1;
    }

    puts("motion pipeline tests: PASS");
    return 0;
}
