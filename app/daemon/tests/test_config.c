#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failure_count = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                             \
            ++failure_count;                                                 \
        }                                                                    \
    } while (0)

static int load_text(const char *text, SenseConfig *config,
                     char *error, size_t error_size)
{
    char path[] = "/tmp/imx6ull-config-XXXXXX";
    size_t length = strlen(text);
    size_t written = 0U;
    int file_descriptor = mkstemp(path);
    int result;

    if (file_descriptor < 0) {
        return -1;
    }
    while (written < length) {
        ssize_t count = write(file_descriptor,
                              text + written,
                              length - written);

        if (count <= 0) {
            close(file_descriptor);
            unlink(path);
            return -1;
        }
        written += (size_t)count;
    }
    if (close(file_descriptor) != 0) {
        unlink(path);
        return -1;
    }

    result = config_load(path, config, error, error_size);
    unlink(path);
    return result;
}

static void test_defaults(void)
{
    SenseConfig config;
    char error[256] = {0};

    CHECK(load_text("{}", &config, error, sizeof(error)) == 0);
    CHECK(config.motion_enabled == 1);
    CHECK(config.motion_sample_fps == 3U);
    CHECK(config.motion_jpeg_scale_denom == 4U);
    CHECK(config.motion_pixel_delta_threshold == 25U);
    CHECK(fabs(config.motion_changed_ratio_threshold - 0.05) < 0.000001);
    CHECK(config.motion_cooldown_ms == 1500U);
    CHECK(strcmp(config.event_log, "events.jsonl") == 0);
}

static void test_explicit_motion_config(void)
{
    static const char json[] =
        "{\"event_log\":\"/tmp/events.jsonl\","
        "\"motion_enabled\":false,"
        "\"motion_sample_fps\":5,"
        "\"motion_jpeg_scale_denom\":8,"
        "\"motion_pixel_delta_threshold\":30,"
        "\"motion_changed_ratio_threshold\":0.075,"
        "\"motion_cooldown_ms\":2500,"
        "\"unknown_decimal\":1.25}";
    SenseConfig config;
    char error[256] = {0};

    CHECK(load_text(json, &config, error, sizeof(error)) == 0);
    CHECK(config.motion_enabled == 0);
    CHECK(config.motion_sample_fps == 5U);
    CHECK(config.motion_jpeg_scale_denom == 8U);
    CHECK(config.motion_pixel_delta_threshold == 30U);
    CHECK(fabs(config.motion_changed_ratio_threshold - 0.075) < 0.000001);
    CHECK(config.motion_cooldown_ms == 2500U);
    CHECK(strcmp(config.event_log, "/tmp/events.jsonl") == 0);
}

static void expect_invalid(const char *json, const char *expected_error)
{
    SenseConfig config;
    char error[256] = {0};

    CHECK(load_text(json, &config, error, sizeof(error)) < 0);
    CHECK(strstr(error, expected_error) != NULL);
}

static void test_invalid_motion_config(void)
{
    expect_invalid("{\"motion_enabled\":1}", "must be a boolean");
    expect_invalid("{\"motion_sample_fps\":0}", "motion_sample_fps");
    expect_invalid("{\"motion_sample_fps\":-1}", "motion_sample_fps");
    expect_invalid("{\"motion_sample_fps\":4294967299}",
                   "motion_sample_fps");
    expect_invalid("{\"http_port\":4294967297}", "http_port");
    expect_invalid("{\"motion_jpeg_scale_denom\":3}",
                   "motion_jpeg_scale_denom");
    expect_invalid("{\"motion_pixel_delta_threshold\":256}",
                   "motion_pixel_delta_threshold");
    expect_invalid("{\"motion_changed_ratio_threshold\":0}",
                   "motion_changed_ratio_threshold");
    expect_invalid("{\"motion_changed_ratio_threshold\":1.1}",
                   "motion_changed_ratio_threshold");
    expect_invalid("{\"motion_cooldown_ms\":0}", "motion_cooldown_ms");
    expect_invalid("{\"event_log\":\"\"}", "event_log cannot be empty");
}

int main(void)
{
    test_defaults();
    test_explicit_motion_config();
    test_invalid_motion_config();

    if (failure_count != 0) {
        fprintf(stderr, "config tests: %d failure(s)\n", failure_count);
        return 1;
    }

    puts("config tests: PASS");
    return 0;
}
