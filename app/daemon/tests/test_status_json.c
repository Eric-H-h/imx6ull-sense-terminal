#include "status_json.h"

#include <stdio.h>
#include <string.h>

static int failure_count = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,        \
                    #condition);                                             \
            ++failure_count;                                                 \
        }                                                                    \
    } while (0)

static void test_active_motion_status(void)
{
    StatusSnapshot snapshot = {
        .degraded = 0,
        .width = 640U,
        .height = 480U,
        .fps = 29.8,
        .frame_count = 1234U,
        .client_count = 2U,
        .motion_enabled = 1,
        .motion_state = 1,
        .motion_score = 0.1875,
        .motion_sample_fps = 2.98,
        .event_count = 7U
    };
    char body[1280];
    int length;

    snprintf(snapshot.device, sizeof(snapshot.device), "cam\"\\one");
    length = status_json_format(&snapshot, body, sizeof(body));

    CHECK(length > 0);
    CHECK((size_t)length == strlen(body));
    CHECK(strstr(body, "\"ok\":true") != NULL);
    CHECK(strstr(body, "\"degraded\":false") != NULL);
    CHECK(strstr(body, "\"device\":\"cam\\\"\\\\one\"") != NULL);
    CHECK(strstr(body, "\"motion_enabled\":true") != NULL);
    CHECK(strstr(body, "\"motion_state\":true") != NULL);
    CHECK(strstr(body, "\"motion_score\":0.187500") != NULL);
    CHECK(strstr(body, "\"motion_sample_fps\":2.98") != NULL);
    CHECK(strstr(body, "\"event_count\":7") != NULL);
    CHECK(strstr(body, "\"last_error\":null") != NULL);
}

static void test_degraded_status_and_error_escape(void)
{
    StatusSnapshot snapshot = {
        .degraded = 1,
        .motion_enabled = 1
    };
    char body[1280];

    snprintf(snapshot.device, sizeof(snapshot.device), "none");
    snprintf(snapshot.last_error, sizeof(snapshot.last_error),
             "camera\n\"missing\"");

    CHECK(status_json_format(&snapshot, body, sizeof(body)) > 0);
    CHECK(strstr(body, "\"ok\":false") != NULL);
    CHECK(strstr(body, "\"degraded\":true") != NULL);
    CHECK(strstr(body, "\"motion_state\":false") != NULL);
    CHECK(strstr(body, "\"last_error\":\"camera\\\"missing\\\"\"") != NULL);
}

static void test_invalid_arguments_and_small_buffer(void)
{
    StatusSnapshot snapshot = {0};
    char body[8] = "stale";

    CHECK(status_json_format(NULL, body, sizeof(body)) < 0);
    CHECK(status_json_format(&snapshot, NULL, sizeof(body)) < 0);
    CHECK(status_json_format(&snapshot, body, 0U) < 0);
    CHECK(status_json_format(&snapshot, body, sizeof(body)) < 0);
    CHECK(body[0] == '\0');
}

int main(void)
{
    test_active_motion_status();
    test_degraded_status_and_error_escape();
    test_invalid_arguments_and_small_buffer();

    if (failure_count != 0) {
        fprintf(stderr, "status json tests: %d failure(s)\n", failure_count);
        return 1;
    }

    puts("status json tests: PASS");
    return 0;
}
