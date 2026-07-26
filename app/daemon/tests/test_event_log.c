#define _POSIX_C_SOURCE 200809L

#include "event_log.h"

#include <errno.h>
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

static MotionEventRecord sample_event(void)
{
    MotionEventRecord event = {
        .timestamp_ms = UINT64_C(1784973600123),
        .sequence = 3821U,
        .score = 0.125,
        .threshold = 0.05,
        .changed_pixels = 2400U,
        .total_pixels = 19200U,
        .cooldown_ms = 3000U
    };

    return event;
}

static void make_test_path(char *path, size_t capacity, const char *suffix)
{
    int length = snprintf(path,
                          capacity,
                          "/tmp/imx6ull-event-log-%ld-%s.jsonl",
                          (long)getpid(),
                          suffix);

    CHECK((length > 0) && ((size_t)length < capacity));
}

static char *read_file(const char *path)
{
    FILE *file;
    long length;
    char *contents;

    file = fopen(path, "rb");
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

static void test_single_event_has_exact_json(void)
{
    static const char expected[] =
        "{\"ts_ms\":1784973600123,\"type\":\"motion\","
        "\"sequence\":3821,\"score\":0.125000,\"threshold\":0.050000,"
        "\"changed_pixels\":2400,\"total_pixels\":19200,"
        "\"cooldown_ms\":3000}\n";
    char path[160];
    char *contents;
    MotionEventRecord event = sample_event();

    make_test_path(path, sizeof(path), "single");
    (void)unlink(path);

    CHECK(event_log_append_motion(path, &event) == EVENT_LOG_OK);
    contents = read_file(path);
    CHECK(contents != NULL);
    if (contents != NULL) {
        CHECK(strcmp(contents, expected) == 0);
        free(contents);
    }

    (void)unlink(path);
}

static void test_append_preserves_existing_event(void)
{
    char path[160];
    char *contents;
    char *first_newline;
    MotionEventRecord first = sample_event();
    MotionEventRecord second = sample_event();

    second.timestamp_ms += 4007U;
    second.sequence = 3941U;
    second.score = 0.083125;
    second.changed_pixels = 1596U;

    make_test_path(path, sizeof(path), "append");
    (void)unlink(path);

    CHECK(event_log_append_motion(path, &first) == EVENT_LOG_OK);
    CHECK(event_log_append_motion(path, &second) == EVENT_LOG_OK);
    contents = read_file(path);
    CHECK(contents != NULL);
    if (contents != NULL) {
        first_newline = strchr(contents, '\n');
        CHECK(first_newline != NULL);
        if (first_newline != NULL) {
            CHECK(strchr(first_newline + 1, '\n') != NULL);
            CHECK(strstr(first_newline + 1,
                         "\"sequence\":3941") != NULL);
        }
        free(contents);
    }

    (void)unlink(path);
}

static void test_invalid_events_do_not_create_file(void)
{
    char path[160];
    MotionEventRecord event = sample_event();

    make_test_path(path, sizeof(path), "invalid");
    (void)unlink(path);

    CHECK(event_log_append_motion(NULL, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    CHECK(event_log_append_motion("", &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    CHECK(event_log_append_motion(path, NULL) ==
          EVENT_LOG_INVALID_ARGUMENT);

    event.score = -0.01;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    event.score = 1.01;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    event.score = NAN;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    event.score = INFINITY;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);

    event = sample_event();
    event.threshold = 0.0;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    event.threshold = NAN;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);

    event = sample_event();
    event.total_pixels = 0U;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    event = sample_event();
    event.changed_pixels = event.total_pixels + 1U;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);
    event = sample_event();
    event.cooldown_ms = 0U;
    CHECK(event_log_append_motion(path, &event) ==
          EVENT_LOG_INVALID_ARGUMENT);

    CHECK(access(path, F_OK) != 0);
}

static void test_open_and_write_errors(void)
{
    char missing_parent[192];
    MotionEventRecord event = sample_event();
    int length;

    length = snprintf(missing_parent,
                      sizeof(missing_parent),
                      "/tmp/imx6ull-event-log-no-parent-%ld/events.jsonl",
                      (long)getpid());
    CHECK((length > 0) && ((size_t)length < sizeof(missing_parent)));
    CHECK(event_log_append_motion(missing_parent, &event) ==
          EVENT_LOG_OPEN_ERROR);

    if (access("/dev/full", W_OK) == 0) {
        CHECK(event_log_append_motion("/dev/full", &event) ==
              EVENT_LOG_WRITE_ERROR);
        CHECK(errno == ENOSPC);
    }
}

static void test_status_strings(void)
{
    CHECK(event_log_status_string(EVENT_LOG_OK) != NULL);
    CHECK(event_log_status_string(EVENT_LOG_INVALID_ARGUMENT) != NULL);
    CHECK(event_log_status_string(EVENT_LOG_FORMAT_ERROR) != NULL);
    CHECK(event_log_status_string(EVENT_LOG_OPEN_ERROR) != NULL);
    CHECK(event_log_status_string(EVENT_LOG_WRITE_ERROR) != NULL);
    CHECK(event_log_status_string(EVENT_LOG_CLOSE_ERROR) != NULL);
    CHECK(event_log_status_string((EventLogStatus)-99) != NULL);
}

static int write_fixture(const char *path)
{
    MotionEventRecord first = sample_event();
    MotionEventRecord second = sample_event();

    if (access(path, F_OK) == 0) {
        fprintf(stderr, "fixture path already exists: %s\n", path);
        return 1;
    }

    second.timestamp_ms += 4007U;
    second.sequence = 3941U;
    second.score = 0.083125;
    second.changed_pixels = 1596U;

    if ((event_log_append_motion(path, &first) != EVENT_LOG_OK) ||
        (event_log_append_motion(path, &second) != EVENT_LOG_OK)) {
        fprintf(stderr, "failed to write fixture: %s\n", path);
        return 1;
    }

    printf("fixture_written: %s (2 events)\n", path);
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc == 3) && (strcmp(argv[1], "--write-fixture") == 0)) {
        return write_fixture(argv[2]);
    }
    if (argc != 1) {
        fprintf(stderr, "usage: %s [--write-fixture FILE]\n", argv[0]);
        return 2;
    }

    test_single_event_has_exact_json();
    test_append_preserves_existing_event();
    test_invalid_events_do_not_create_file();
    test_open_and_write_errors();
    test_status_strings();

    if (failure_count != 0) {
        fprintf(stderr, "event log tests: %d failure(s)\n", failure_count);
        return 1;
    }

    puts("event log tests: PASS");
    return 0;
}
