#ifndef IMX6ULL_JPEG_DECODER_H
#define IMX6ULL_JPEG_DECODER_H

#include <stddef.h>

typedef struct {
    unsigned char *pixels;
    size_t capacity;
    unsigned int width;
    unsigned int height;
    size_t stride;
} JpegGrayFrame;

typedef enum {
    JPEG_GRAY_OK = 0,
    JPEG_GRAY_INVALID_ARGUMENT = -1,
    JPEG_GRAY_INVALID_IMAGE = -2,
    JPEG_GRAY_NO_MEMORY = -3,
    JPEG_GRAY_SIZE_OVERFLOW = -4
} JpegGrayResult;

/*
 * Decode an in-memory JPEG into an 8-bit grayscale frame.
 *
 * scale_denom must be 1, 2, 4, or 8. The caller initializes output to zero
 * before its first use and releases it with jpeg_gray_frame_release(). The
 * decoder owns and may reuse output->pixels. Only read pixels after a
 * JPEG_GRAY_OK result. A failed decode leaves the output metadata unchanged,
 * but may overwrite pixels in an existing reusable buffer.
 */
JpegGrayResult jpeg_gray_decode(const unsigned char *jpeg_data,
                                size_t jpeg_size,
                                unsigned int scale_denom,
                                JpegGrayFrame *output);

void jpeg_gray_frame_release(JpegGrayFrame *frame);
const char *jpeg_gray_result_string(JpegGrayResult result);

#endif
