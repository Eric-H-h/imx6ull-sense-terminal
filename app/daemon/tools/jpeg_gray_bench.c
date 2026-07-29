#define _POSIX_C_SOURCE 200809L

#include "jpeg_decoder.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NS_PER_SECOND UINT64_C(1000000000)

typedef struct {
    const char *input_path;
    unsigned int scale_denom;
    unsigned int target_fps;
    unsigned int duration_seconds;
} BenchOptions;

static void print_usage(const char *program)
{
    fprintf(stderr,
            "usage: %s --input FILE [--scale 1|2|4|8] "
            "[--fps 1..120] [--seconds 1..3600]\n",
            program);
}

static int parse_unsigned(const char *text, unsigned int minimum,
                          unsigned int maximum, unsigned int *value)
{
    char *end;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0') ||
        (parsed < minimum) || (parsed > maximum)) {
        return -1;
    }

    *value = (unsigned int)parsed;
    return 0;
}

static int parse_options(int argc, char **argv, BenchOptions *options)
{
    options->input_path = NULL;
    options->scale_denom = 4U;
    options->target_fps = 3U;
    options->duration_seconds = 30U;

    for (int index = 1; index < argc; ++index) {
        const char *name = argv[index];

        if ((strcmp(name, "--help") == 0) ||
            (strcmp(name, "-h") == 0)) {
            print_usage(argv[0]);
            return 1;
        }
        if (++index >= argc) {
            return -1;
        }

        if (strcmp(name, "--input") == 0) {
            options->input_path = argv[index];
        } else if (strcmp(name, "--scale") == 0) {
            if (parse_unsigned(argv[index], 1U, 8U,
                               &options->scale_denom) < 0) {
                return -1;
            }
        } else if (strcmp(name, "--fps") == 0) {
            if (parse_unsigned(argv[index], 1U, 120U,
                               &options->target_fps) < 0) {
                return -1;
            }
        } else if (strcmp(name, "--seconds") == 0) {
            if (parse_unsigned(argv[index], 1U, 3600U,
                               &options->duration_seconds) < 0) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    if ((options->input_path == NULL) ||
        ((options->scale_denom != 1U) &&
         (options->scale_denom != 2U) &&
         (options->scale_denom != 4U) &&
         (options->scale_denom != 8U))) {
        return -1;
    }
    return 0;
}

static int read_file(const char *path, unsigned char **data, size_t *size)
{
    FILE *file;
    long length;
    unsigned char *buffer;

    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "open failed: %s: %s\n", path, strerror(errno));
        return -1;
    }

    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) <= 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0)) {
        fprintf(stderr, "cannot determine input size: %s\n", path);
        fclose(file);
        return -1;
    }

    buffer = malloc((size_t)length);
    if (buffer == NULL) {
        fprintf(stderr, "cannot allocate %ld input bytes\n", length);
        fclose(file);
        return -1;
    }

    if (fread(buffer, 1U, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "cannot read complete input: %s\n", path);
        free(buffer);
        fclose(file);
        return -1;
    }

    fclose(file);
    *data = buffer;
    *size = (size_t)length;
    return 0;
}

static int monotonic_now_ns(uint64_t *value)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
    }

    *value = ((uint64_t)now.tv_sec * NS_PER_SECOND) +
             (uint64_t)now.tv_nsec;
    return 0;
}

static struct timespec ns_to_timespec(uint64_t value)
{
    struct timespec result;

    result.tv_sec = (time_t)(value / NS_PER_SECOND);
    result.tv_nsec = (long)(value % NS_PER_SECOND);
    return result;
}

static int sleep_until(uint64_t deadline_ns)
{
    struct timespec deadline = ns_to_timespec(deadline_ns);
    int result;

    do {
        result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                 &deadline, NULL);
    } while (result == EINTR);

    return result;
}

static int run_benchmark(const BenchOptions *options,
                         const unsigned char *jpeg_data, size_t jpeg_size)
{
    JpegGrayFrame frame = {0};
    JpegGrayResult last_result = JPEG_GRAY_OK;
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t period_ns = NS_PER_SECOND / options->target_fps;
    uint64_t target_samples =
        (uint64_t)options->target_fps * options->duration_seconds;
    uint64_t success_count = 0U;
    uint64_t failure_count = 0U;
    uint64_t total_decode_ns = 0U;
    uint64_t maximum_decode_ns = 0U;

    if (monotonic_now_ns(&start_ns) < 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        return 1;
    }

    for (uint64_t index = 0U; index < target_samples; ++index) {
        uint64_t decode_start_ns;
        uint64_t decode_end_ns;
        uint64_t decode_ns;
        int sleep_result;

        sleep_result = sleep_until(start_ns + (index * period_ns));
        if (sleep_result != 0) {
            fprintf(stderr, "clock_nanosleep failed: %s\n",
                    strerror(sleep_result));
            jpeg_gray_frame_release(&frame);
            return 1;
        }
        if (monotonic_now_ns(&decode_start_ns) < 0) {
            fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
            jpeg_gray_frame_release(&frame);
            return 1;
        }

        last_result = jpeg_gray_decode(jpeg_data, jpeg_size,
                                       options->scale_denom, &frame);

        if (monotonic_now_ns(&decode_end_ns) < 0) {
            fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
            jpeg_gray_frame_release(&frame);
            return 1;
        }
        decode_ns = decode_end_ns - decode_start_ns;

        if (last_result == JPEG_GRAY_OK) {
            ++success_count;
            total_decode_ns += decode_ns;
            if (decode_ns > maximum_decode_ns) {
                maximum_decode_ns = decode_ns;
            }
        } else {
            ++failure_count;
        }
    }

    end_ns = start_ns +
             ((uint64_t)options->duration_seconds * NS_PER_SECOND);
    {
        int sleep_result = sleep_until(end_ns);

        if (sleep_result != 0) {
            fprintf(stderr, "final clock_nanosleep failed: %s\n",
                    strerror(sleep_result));
            jpeg_gray_frame_release(&frame);
            return 1;
        }
    }
    if (monotonic_now_ns(&end_ns) < 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        jpeg_gray_frame_release(&frame);
        return 1;
    }

    printf("input_size: %zu bytes\n", jpeg_size);
    printf("gray_size: %ux%u\n", frame.width, frame.height);
    printf("scale: 1/%u\n", options->scale_denom);
    printf("target_fps: %u\n", options->target_fps);
    printf("duration_seconds: %u\n", options->duration_seconds);
    printf("decode_success: %" PRIu64 "\n", success_count);
    printf("decode_failed: %" PRIu64 "\n", failure_count);
    if (success_count > 0U) {
        printf("average_decode_ms: %.3f\n",
               (double)total_decode_ns / (double)success_count / 1000000.0);
        printf("max_decode_ms: %.3f\n",
               (double)maximum_decode_ns / 1000000.0);
    }
    printf("actual_sample_fps: %.3f\n",
           (double)target_samples * (double)NS_PER_SECOND /
           (double)(end_ns - start_ns));

    if (failure_count != 0U) {
        fprintf(stderr, "last decode error: %s\n",
                jpeg_gray_result_string(last_result));
    }

    jpeg_gray_frame_release(&frame);
    return failure_count == 0U ? 0 : 1;
}

int main(int argc, char **argv)
{
    BenchOptions options;
    unsigned char *jpeg_data = NULL;
    size_t jpeg_size = 0U;
    int parse_result;
    int result;

    parse_result = parse_options(argc, argv, &options);
    if (parse_result != 0) {
        if (parse_result < 0) {
            print_usage(argv[0]);
        }
        return parse_result > 0 ? 0 : 2;
    }

    if (read_file(options.input_path, &jpeg_data, &jpeg_size) < 0) {
        return 1;
    }

    result = run_benchmark(&options, jpeg_data, jpeg_size);
    free(jpeg_data);
    return result;
}
