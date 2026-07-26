#include "jpeg_decoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void create_test_jpeg(unsigned char **jpeg_data,
                             unsigned long *jpeg_size)
{
    struct jpeg_compress_struct encoder;
    struct jpeg_error_mgr error;
    unsigned char row[640];

    encoder.err = jpeg_std_error(&error);
    jpeg_create_compress(&encoder);
    jpeg_mem_dest(&encoder, jpeg_data, jpeg_size);

    encoder.image_width = 640U;
    encoder.image_height = 480U;
    encoder.input_components = 1;
    encoder.in_color_space = JCS_GRAYSCALE;
    jpeg_set_defaults(&encoder);
    jpeg_set_quality(&encoder, 100, TRUE);
    jpeg_start_compress(&encoder, TRUE);

    while (encoder.next_scanline < encoder.image_height) {
        JSAMPROW row_pointer = row;

        for (unsigned int column = 0U; column < 640U; ++column) {
            row[column] = (unsigned char)((column * 255U) / 639U);
        }
        jpeg_write_scanlines(&encoder, &row_pointer, 1U);
    }

    jpeg_finish_compress(&encoder);
    jpeg_destroy_compress(&encoder);
}

static void test_invalid_arguments(void)
{
    static const unsigned char fake_jpeg[] = {0xffU, 0xd8U, 0xffU, 0xd9U};
    JpegGrayFrame frame = {0};

    CHECK(jpeg_gray_decode(NULL, sizeof(fake_jpeg), 4U, &frame) ==
          JPEG_GRAY_INVALID_ARGUMENT);
    CHECK(jpeg_gray_decode(fake_jpeg, 0U, 4U, &frame) ==
          JPEG_GRAY_INVALID_ARGUMENT);
    CHECK(jpeg_gray_decode(fake_jpeg, sizeof(fake_jpeg), 3U, &frame) ==
          JPEG_GRAY_INVALID_ARGUMENT);
    CHECK(jpeg_gray_decode(fake_jpeg, sizeof(fake_jpeg), 4U, NULL) ==
          JPEG_GRAY_INVALID_ARGUMENT);
}

static void test_valid_jpeg_and_buffer_reuse(void)
{
    unsigned char *jpeg_data = NULL;
    unsigned long jpeg_size = 0U;
    JpegGrayFrame frame = {0};
    JpegGrayResult result;
    unsigned char *first_buffer;

    create_test_jpeg(&jpeg_data, &jpeg_size);
    CHECK(jpeg_data != NULL);
    CHECK(jpeg_size > 0U);
    if (jpeg_data == NULL) {
        return;
    }

    result = jpeg_gray_decode(jpeg_data, (size_t)jpeg_size, 4U, &frame);
    CHECK(result == JPEG_GRAY_OK);
    if (result != JPEG_GRAY_OK) {
        free(jpeg_data);
        return;
    }

    CHECK(frame.pixels != NULL);
    CHECK(frame.capacity >= (160U * 120U));
    CHECK(frame.width == 160U);
    CHECK(frame.height == 120U);
    CHECK(frame.stride == 160U);
    CHECK(frame.pixels[0] < frame.pixels[159]);

    first_buffer = frame.pixels;
    result = jpeg_gray_decode(jpeg_data, (size_t)jpeg_size, 8U, &frame);
    CHECK(result == JPEG_GRAY_OK);
    if (result == JPEG_GRAY_OK) {
        CHECK(frame.pixels == first_buffer);
        CHECK(frame.width == 80U);
        CHECK(frame.height == 60U);
        CHECK(frame.stride == 80U);
    }

    jpeg_gray_frame_release(&frame);
    free(jpeg_data);
}

static void test_invalid_jpeg_does_not_exit(void)
{
    static const unsigned char invalid_jpeg[] = {
        0x6eU, 0x6fU, 0x74U, 0x2dU, 0x6aU, 0x70U, 0x65U, 0x67U
    };
    JpegGrayFrame frame = {0};

    CHECK(jpeg_gray_decode(invalid_jpeg, sizeof(invalid_jpeg), 4U, &frame) ==
          JPEG_GRAY_INVALID_IMAGE);
    CHECK(frame.pixels == NULL);
    CHECK(frame.capacity == 0U);
}

static void test_frame_release(void)
{
    JpegGrayFrame frame = {0};

    frame.pixels = malloc(16U);
    CHECK(frame.pixels != NULL);
    if (frame.pixels == NULL) {
        return;
    }

    frame.capacity = 16U;
    frame.width = 4U;
    frame.height = 4U;
    frame.stride = 4U;

    jpeg_gray_frame_release(&frame);
    CHECK(frame.pixels == NULL);
    CHECK(frame.capacity == 0U);
    CHECK(frame.width == 0U);
    CHECK(frame.height == 0U);
    CHECK(frame.stride == 0U);

    jpeg_gray_frame_release(&frame);
    jpeg_gray_frame_release(NULL);
}

static void test_result_strings(void)
{
    CHECK(jpeg_gray_result_string(JPEG_GRAY_OK) != NULL);
    CHECK(jpeg_gray_result_string(JPEG_GRAY_INVALID_ARGUMENT) != NULL);
    CHECK(jpeg_gray_result_string(JPEG_GRAY_INVALID_IMAGE) != NULL);
    CHECK(jpeg_gray_result_string((JpegGrayResult)-99) != NULL);
}

static int write_test_fixture(const char *path)
{
    unsigned char *jpeg_data = NULL;
    unsigned long jpeg_size = 0U;
    FILE *file;

    create_test_jpeg(&jpeg_data, &jpeg_size);
    if (jpeg_data == NULL) {
        return 1;
    }

    file = fopen(path, "wb");
    if (file == NULL) {
        free(jpeg_data);
        return 1;
    }

    if (fwrite(jpeg_data, 1U, (size_t)jpeg_size, file) !=
        (size_t)jpeg_size) {
        fclose(file);
        free(jpeg_data);
        return 1;
    }

    fclose(file);
    free(jpeg_data);
    printf("fixture_written: %s (%lu bytes)\n", path, jpeg_size);
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc == 3) && (strcmp(argv[1], "--write-fixture") == 0)) {
        return write_test_fixture(argv[2]);
    }
    if (argc != 1) {
        fprintf(stderr, "usage: %s [--write-fixture FILE]\n", argv[0]);
        return 2;
    }

    test_invalid_arguments();
    test_valid_jpeg_and_buffer_reuse();
    test_invalid_jpeg_does_not_exit();
    test_frame_release();
    test_result_strings();

    if (failure_count != 0) {
        fprintf(stderr, "jpeg decoder tests: %d failure(s)\n", failure_count);
        return 1;
    }

    puts("jpeg decoder tests: PASS");
    return 0;
}
