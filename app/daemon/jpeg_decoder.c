#include "jpeg_decoder.h"

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <jpeglib.h>

typedef struct {
    struct jpeg_error_mgr base;
    jmp_buf jump_buffer;
} JpegErrorManager;

static void decoder_error_exit(j_common_ptr common)
{
    JpegErrorManager *error = (JpegErrorManager *)common->err;

    longjmp(error->jump_buffer, 1);
}

static int scale_is_supported(unsigned int scale_denom)
{
    return (scale_denom == 1U) || (scale_denom == 2U) ||
           (scale_denom == 4U) || (scale_denom == 8U);
}

JpegGrayResult jpeg_gray_decode(const unsigned char *jpeg_data,
                                size_t jpeg_size,
                                unsigned int scale_denom,
                                JpegGrayFrame *output)
{
    struct jpeg_decompress_struct decoder;
    JpegErrorManager error;
    unsigned char *volatile allocated_pixels = NULL;
    volatile int decoder_created = 0;
    unsigned char *target;
    unsigned int decoded_width;
    unsigned int decoded_height;
    size_t required_size;
    size_t stride;

    if ((jpeg_data == NULL) || (jpeg_size == 0U) || (output == NULL) ||
        !scale_is_supported(scale_denom)) {
        return JPEG_GRAY_INVALID_ARGUMENT;
    }

    decoder.err = jpeg_std_error(&error.base);
    error.base.error_exit = decoder_error_exit;

    if (setjmp(error.jump_buffer) != 0) {
        if (decoder_created) {
            jpeg_destroy_decompress(&decoder);
        }
        free((void *)allocated_pixels);
        return JPEG_GRAY_INVALID_IMAGE;
    }

    jpeg_create_decompress(&decoder);
    decoder_created = 1;
    jpeg_mem_src(&decoder, jpeg_data, (unsigned long)jpeg_size);

    if (jpeg_read_header(&decoder, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&decoder);
        return JPEG_GRAY_INVALID_IMAGE;
    }

    decoder.out_color_space = JCS_GRAYSCALE;
    decoder.scale_num = 1U;
    decoder.scale_denom = scale_denom;
    jpeg_start_decompress(&decoder);

    if ((decoder.output_width == 0U) || (decoder.output_height == 0U) ||
        (decoder.output_components != 1)) {
        jpeg_destroy_decompress(&decoder);
        return JPEG_GRAY_INVALID_IMAGE;
    }

    decoded_width = (unsigned int)decoder.output_width;
    decoded_height = (unsigned int)decoder.output_height;
    stride = (size_t)decoded_width;
    if (stride > (SIZE_MAX / (size_t)decoded_height)) {
        jpeg_destroy_decompress(&decoder);
        return JPEG_GRAY_SIZE_OVERFLOW;
    }
    required_size = stride * (size_t)decoded_height;

    if ((output->pixels == NULL) || (output->capacity < required_size)) {
        allocated_pixels = malloc(required_size);
        if (allocated_pixels == NULL) {
            jpeg_destroy_decompress(&decoder);
            return JPEG_GRAY_NO_MEMORY;
        }
        target = (unsigned char *)allocated_pixels;
    } else {
        target = output->pixels;
    }

    while (decoder.output_scanline < decoder.output_height) {
        JSAMPROW row = target +
                       ((size_t)decoder.output_scanline * stride);

        if (jpeg_read_scanlines(&decoder, &row, 1U) != 1U) {
            jpeg_destroy_decompress(&decoder);
            free((void *)allocated_pixels);
            return JPEG_GRAY_INVALID_IMAGE;
        }
    }

    jpeg_finish_decompress(&decoder);
    jpeg_destroy_decompress(&decoder);

    if (allocated_pixels != NULL) {
        free(output->pixels);
        output->pixels = (unsigned char *)allocated_pixels;
        output->capacity = required_size;
    }
    output->width = decoded_width;
    output->height = decoded_height;
    output->stride = stride;

    return JPEG_GRAY_OK;
}

void jpeg_gray_frame_release(JpegGrayFrame *frame)
{
    if (frame == NULL) {
        return;
    }

    free(frame->pixels);
    frame->pixels = NULL;
    frame->capacity = 0U;
    frame->width = 0U;
    frame->height = 0U;
    frame->stride = 0U;
}

const char *jpeg_gray_result_string(JpegGrayResult result)
{
    switch (result) {
    case JPEG_GRAY_OK:
        return "success";
    case JPEG_GRAY_INVALID_ARGUMENT:
        return "invalid argument";
    case JPEG_GRAY_INVALID_IMAGE:
        return "invalid JPEG image";
    case JPEG_GRAY_NO_MEMORY:
        return "out of memory";
    case JPEG_GRAY_SIZE_OVERFLOW:
        return "decoded image is too large";
    default:
        return "unknown JPEG decode result";
    }
}
