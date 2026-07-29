#include "motion_detector.h"

#include <stdint.h>
#include <stdio.h>

static int failure_count = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                             \
            ++failure_count;                                                 \
        }                                                                    \
    } while (0)

static const MotionThresholds default_thresholds = {
    .pixel_delta_threshold = 20U,
    .changed_ratio_threshold = 0.25
};

static void test_first_frame_and_identical_frame(void)
{
    static const unsigned char frame[] = {
        10U, 20U, 30U, 40U,
        50U, 60U, 70U, 80U
    };
    MotionDetector detector = {0};
    MotionResult result;

    CHECK(motion_detector_process(&detector, frame, 4U, 2U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.baseline_created == 1);
    CHECK(result.total_pixels == 8U);
    CHECK(result.changed_pixels == 0U);
    CHECK(result.changed_ratio == 0.0);
    CHECK(result.motion_detected == 0);

    CHECK(motion_detector_process(&detector, frame, 4U, 2U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.baseline_created == 0);
    CHECK(result.changed_pixels == 0U);
    CHECK(result.changed_ratio == 0.0);
    CHECK(result.motion_detected == 0);

    motion_detector_destroy(&detector);
}

static void test_small_changes_stay_below_pixel_threshold(void)
{
    static const unsigned char first[] = {100U, 100U, 100U, 100U};
    static const unsigned char second[] = {119U, 81U, 110U, 90U};
    MotionDetector detector = {0};
    MotionResult result;

    CHECK(motion_detector_process(&detector, first, 4U, 1U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(motion_detector_process(&detector, second, 4U, 1U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.changed_pixels == 0U);
    CHECK(result.motion_detected == 0);

    motion_detector_destroy(&detector);
}

static void test_obvious_change_detects_motion(void)
{
    static const unsigned char first[] = {
        0U, 0U, 0U, 0U,
        0U, 0U, 0U, 0U
    };
    static const unsigned char second[] = {
        50U, 50U, 50U, 50U,
        0U, 0U, 0U, 0U
    };
    MotionDetector detector = {0};
    MotionResult result;

    CHECK(motion_detector_process(&detector, first, 4U, 2U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(motion_detector_process(&detector, second, 4U, 2U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.total_pixels == 8U);
    CHECK(result.changed_pixels == 4U);
    CHECK(result.changed_ratio == 0.5);
    CHECK(result.motion_detected == 1);

    motion_detector_destroy(&detector);
}

static void test_threshold_boundaries_are_inclusive(void)
{
    static const unsigned char first[] = {10U, 10U, 10U, 10U};
    static const unsigned char second[] = {30U, 10U, 10U, 10U};
    MotionDetector detector = {0};
    MotionResult result;

    CHECK(motion_detector_process(&detector, first, 4U, 1U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(motion_detector_process(&detector, second, 4U, 1U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.changed_pixels == 1U);
    CHECK(result.changed_ratio == 0.25);
    CHECK(result.motion_detected == 1);

    motion_detector_destroy(&detector);
}

static void test_stride_padding_is_ignored(void)
{
    static const unsigned char first[] = {
        10U, 20U, 0U, 0U,
        30U, 40U, 0U, 0U
    };
    static const unsigned char second[] = {
        10U, 20U, 255U, 255U,
        30U, 40U, 255U, 255U
    };
    MotionDetector detector = {0};
    MotionResult result;

    CHECK(motion_detector_process(&detector, first, 2U, 2U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(motion_detector_process(&detector, second, 2U, 2U, 4U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.total_pixels == 4U);
    CHECK(result.changed_pixels == 0U);
    CHECK(result.motion_detected == 0);

    motion_detector_destroy(&detector);
}

static void test_size_change_and_reset_rebuild_baseline(void)
{
    static const unsigned char frame_2x2[] = {0U, 0U, 0U, 0U};
    static const unsigned char frame_3x1[] = {255U, 255U, 255U};
    MotionDetector detector = {0};
    MotionResult result;
    unsigned char *allocated_buffer;

    CHECK(motion_detector_process(&detector, frame_2x2, 2U, 2U, 2U,
                                  default_thresholds, &result) == MOTION_OK);
    allocated_buffer = detector.previous_pixels;
    CHECK(allocated_buffer != NULL);

    CHECK(motion_detector_process(&detector, frame_3x1, 3U, 1U, 3U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.baseline_created == 1);
    CHECK(result.motion_detected == 0);
    CHECK(detector.previous_pixels == allocated_buffer);

    motion_detector_reset(&detector);
    CHECK(detector.baseline_ready == 0);
    CHECK(detector.previous_pixels == allocated_buffer);
    CHECK(detector.capacity >= 4U);

    CHECK(motion_detector_process(&detector, frame_3x1, 3U, 1U, 3U,
                                  default_thresholds, &result) == MOTION_OK);
    CHECK(result.baseline_created == 1);
    CHECK(result.motion_detected == 0);

    motion_detector_destroy(&detector);
    CHECK(detector.previous_pixels == NULL);
    CHECK(detector.capacity == 0U);
    motion_detector_destroy(&detector);
    motion_detector_destroy(NULL);
}

static void test_invalid_arguments_and_overflow(void)
{
    static const unsigned char frame[] = {0U, 0U, 0U, 0U};
    MotionDetector detector = {0};
    MotionResult result;
    MotionThresholds invalid_thresholds = default_thresholds;

    CHECK(motion_detector_process(NULL, frame, 2U, 2U, 2U,
                                  default_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    CHECK(motion_detector_process(&detector, NULL, 2U, 2U, 2U,
                                  default_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    CHECK(motion_detector_process(&detector, frame, 2U, 2U, 2U,
                                  default_thresholds, NULL) ==
          MOTION_INVALID_ARGUMENT);
    CHECK(motion_detector_process(&detector, frame, 0U, 2U, 2U,
                                  default_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    CHECK(motion_detector_process(&detector, frame, 2U, 0U, 2U,
                                  default_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    CHECK(motion_detector_process(&detector, frame, 2U, 2U, 1U,
                                  default_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);

    invalid_thresholds.pixel_delta_threshold = 0U;
    CHECK(motion_detector_process(&detector, frame, 2U, 2U, 2U,
                                  invalid_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    invalid_thresholds.pixel_delta_threshold = 256U;
    CHECK(motion_detector_process(&detector, frame, 2U, 2U, 2U,
                                  invalid_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    invalid_thresholds = default_thresholds;
    invalid_thresholds.changed_ratio_threshold = 0.0;
    CHECK(motion_detector_process(&detector, frame, 2U, 2U, 2U,
                                  invalid_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);
    invalid_thresholds.changed_ratio_threshold = 1.01;
    CHECK(motion_detector_process(&detector, frame, 2U, 2U, 2U,
                                  invalid_thresholds, &result) ==
          MOTION_INVALID_ARGUMENT);

    CHECK(motion_detector_process(&detector, frame, 1U, 2U, SIZE_MAX,
                                  default_thresholds, &result) ==
          MOTION_SIZE_OVERFLOW);
    motion_detector_destroy(&detector);
}

static void test_status_strings(void)
{
    CHECK(motion_status_string(MOTION_OK) != NULL);
    CHECK(motion_status_string(MOTION_INVALID_ARGUMENT) != NULL);
    CHECK(motion_status_string(MOTION_NO_MEMORY) != NULL);
    CHECK(motion_status_string(MOTION_SIZE_OVERFLOW) != NULL);
    CHECK(motion_status_string((MotionStatus)-99) != NULL);
}

int main(void)
{
    test_first_frame_and_identical_frame();
    test_small_changes_stay_below_pixel_threshold();
    test_obvious_change_detects_motion();
    test_threshold_boundaries_are_inclusive();
    test_stride_padding_is_ignored();
    test_size_change_and_reset_rebuild_baseline();
    test_invalid_arguments_and_overflow();
    test_status_strings();

    if (failure_count != 0) {
        fprintf(stderr, "motion detector tests: %d failure(s)\n",
                failure_count);
        return 1;
    }

    puts("motion detector tests: PASS");
    return 0;
}
