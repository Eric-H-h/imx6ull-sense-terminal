#define _POSIX_C_SOURCE 200809L

#include "sense.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#define VIDEO_NODE_LIMIT 64
#define CAPTURE_BUFFER_COUNT 4

typedef struct {
    void *start;
    size_t length;
} MappedBuffer;

static int is_uvcvideo_node(const char *path)
{
    const char *node = strrchr(path, '/');
    const char *driver;
    char sysfs_path[128];
    char driver_target[256];
    ssize_t length;
    int path_size;

    node = (node == NULL) ? path : node + 1;
    if ((strncmp(node, "video", 5) != 0) || (node[5] == '\0')) {
        return 0;
    }
    for (const char *cursor = node + 5; *cursor != '\0'; ++cursor) {
        if (!isdigit((unsigned char)*cursor)) {
            return 0;
        }
    }

    path_size = snprintf(sysfs_path, sizeof(sysfs_path),
                         "/sys/class/video4linux/%s/device/driver", node);
    if ((path_size < 0) || ((size_t)path_size >= sizeof(sysfs_path))) {
        return 0;
    }

    length = readlink(sysfs_path, driver_target, sizeof(driver_target) - 1);
    if (length < 0) {
        return 0;
    }
    driver_target[length] = '\0';
    driver = strrchr(driver_target, '/');
    driver = (driver == NULL) ? driver_target : driver + 1;
    return strcmp(driver, "uvcvideo") == 0;
}

static int xioctl(int fd, unsigned long request, void *argument)
{
    int result;

    do {
        result = ioctl(fd, request, argument);
    } while ((result < 0) && (errno == EINTR));
    return result;
}

static uint32_t device_capabilities(const struct v4l2_capability *capability)
{
    if ((capability->capabilities & V4L2_CAP_DEVICE_CAPS) != 0) {
        return capability->device_caps;
    }
    return capability->capabilities;
}

static int open_uvc_capture(const char *path)
{
    struct v4l2_capability capability;
    uint32_t caps;
    int fd;

    if (!is_uvcvideo_node(path)) {
        errno = ENODEV;
        return -1;
    }

    fd = open(path, O_RDWR | O_NONBLOCK);

    if (fd < 0) {
        return -1;
    }

    memset(&capability, 0, sizeof(capability));
    if (xioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
        close(fd);
        return -1;
    }

    caps = device_capabilities(&capability);
    if ((strcmp((const char *)capability.driver, "uvcvideo") != 0) ||
        ((caps & V4L2_CAP_VIDEO_CAPTURE) == 0) ||
        ((caps & V4L2_CAP_STREAMING) == 0)) {
        close(fd);
        return -1;
    }
    return fd;
}

static int select_camera(const char *selector, char *selected,
                         size_t selected_size)
{
    int fd;

    if (strcmp(selector, "auto") != 0) {
        fd = open_uvc_capture(selector);
        if (fd >= 0) {
            snprintf(selected, selected_size, "%s", selector);
        }
        return fd;
    }

    for (int index = 0; index < VIDEO_NODE_LIMIT; ++index) {
        char path[SENSE_DEVICE_MAX];

        snprintf(path, sizeof(path), "/dev/video%d", index);
        fd = open_uvc_capture(path);
        if (fd >= 0) {
            snprintf(selected, selected_size, "%s", path);
            return fd;
        }
    }
    return -1;
}

static void retry_delay(AppState *state)
{
    struct timespec delay = {0, 200000000L};

    for (int i = 0; i < 10 && !state_should_stop(state); ++i) {
        nanosleep(&delay, NULL);
    }
}

static int capture_once(CaptureContext *context, char *error,
                        size_t error_size)
{
    MappedBuffer buffers[CAPTURE_BUFFER_COUNT];
    struct v4l2_requestbuffers request;
    struct v4l2_format format;
    struct v4l2_streamparm parameters;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    char selected[SENSE_DEVICE_MAX] = "none";
    unsigned int mapped_count = 0;
    int streaming = 0;
    int fd = -1;
    int result = -1;

    memset(buffers, 0, sizeof(buffers));
    fd = select_camera(context->config->device, selected, sizeof(selected));
    if (fd < 0) {
        snprintf(error, error_size,
                 "no UVC Video Capture node matched selector '%s'",
                 context->config->device);
        state_set_degraded(context->state, "none", error);
        return -1;
    }

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = context->config->width;
    format.fmt.pix.height = context->config->height;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    if (xioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        snprintf(error, error_size, "%s VIDIOC_S_FMT failed: %s",
                 selected, strerror(errno));
        goto cleanup;
    }
    if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG) {
        snprintf(error, error_size, "%s did not accept MJPG", selected);
        goto cleanup;
    }

    memset(&parameters, 0, sizeof(parameters));
    parameters.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parameters.parm.capture.timeperframe.numerator = 1;
    parameters.parm.capture.timeperframe.denominator = context->config->fps_limit;
    if (xioctl(fd, VIDIOC_S_PARM, &parameters) < 0) {
        printf("capture warning: %s VIDIOC_S_PARM failed: %s\n",
               selected, strerror(errno));
    }

    memset(&request, 0, sizeof(request));
    request.count = CAPTURE_BUFFER_COUNT;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_REQBUFS, &request) < 0) {
        snprintf(error, error_size, "%s VIDIOC_REQBUFS failed: %s",
                 selected, strerror(errno));
        goto cleanup;
    }
    if (request.count < 2) {
        snprintf(error, error_size, "%s returned fewer than two mmap buffers",
                 selected);
        goto cleanup;
    }

    mapped_count = request.count;
    if (mapped_count > CAPTURE_BUFFER_COUNT) {
        mapped_count = CAPTURE_BUFFER_COUNT;
    }

    for (unsigned int index = 0; index < mapped_count; ++index) {
        struct v4l2_buffer buffer;

        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = index;
        if (xioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            snprintf(error, error_size, "%s VIDIOC_QUERYBUF failed: %s",
                     selected, strerror(errno));
            goto cleanup;
        }

        buffers[index].length = buffer.length;
        buffers[index].start = mmap(NULL, buffer.length,
                                    PROT_READ | PROT_WRITE, MAP_SHARED,
                                    fd, buffer.m.offset);
        if (buffers[index].start == MAP_FAILED) {
            buffers[index].start = NULL;
            snprintf(error, error_size, "%s mmap failed: %s",
                     selected, strerror(errno));
            goto cleanup;
        }
    }

    for (unsigned int index = 0; index < mapped_count; ++index) {
        struct v4l2_buffer buffer;

        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = index;
        if (xioctl(fd, VIDIOC_QBUF, &buffer) < 0) {
            snprintf(error, error_size, "%s VIDIOC_QBUF failed: %s",
                     selected, strerror(errno));
            goto cleanup;
        }
    }

    if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        snprintf(error, error_size, "%s VIDIOC_STREAMON failed: %s",
                 selected, strerror(errno));
        goto cleanup;
    }
    streaming = 1;
    state_set_capture_active(context->state, selected,
                             format.fmt.pix.width, format.fmt.pix.height);
    printf("capture active: %s MJPG %ux%u\n", selected,
           format.fmt.pix.width, format.fmt.pix.height);

    while (!state_should_stop(context->state)) {
        struct v4l2_buffer buffer;
        fd_set descriptors;
        struct timeval timeout = {2, 0};
        unsigned char *frame;
        int ready;

        FD_ZERO(&descriptors);
        FD_SET(fd, &descriptors);
        ready = select(fd + 1, &descriptors, NULL, NULL, &timeout);
        if (ready == 0) {
            snprintf(error, error_size, "%s frame timeout", selected);
            goto cleanup;
        }
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            snprintf(error, error_size, "%s select failed: %s",
                     selected, strerror(errno));
            goto cleanup;
        }

        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        if (xioctl(fd, VIDIOC_DQBUF, &buffer) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            snprintf(error, error_size, "%s VIDIOC_DQBUF failed: %s",
                     selected, strerror(errno));
            goto cleanup;
        }

        if ((buffer.index >= mapped_count) ||
            (buffer.bytesused > buffers[buffer.index].length)) {
            snprintf(error, error_size, "%s returned an invalid buffer",
                     selected);
            goto cleanup;
        }

        frame = buffers[buffer.index].start;
        if ((buffer.bytesused < 2) || (frame[0] != 0xff) || (frame[1] != 0xd8)) {
            snprintf(error, error_size,
                     "%s returned a non-JPEG frame while configured for MJPG",
                     selected);
            goto cleanup;
        }

        if (state_publish_frame(context->state, frame, buffer.bytesused) < 0) {
            snprintf(error, error_size, "out of memory copying camera frame");
            goto cleanup;
        }

        if (xioctl(fd, VIDIOC_QBUF, &buffer) < 0) {
            snprintf(error, error_size, "%s requeue failed: %s",
                     selected, strerror(errno));
            goto cleanup;
        }
    }

    result = 0;

cleanup:
    if (streaming) {
        xioctl(fd, VIDIOC_STREAMOFF, &type);
    }
    for (unsigned int index = 0; index < mapped_count; ++index) {
        if (buffers[index].start != NULL) {
            munmap(buffers[index].start, buffers[index].length);
        }
    }
    if (fd >= 0) {
        close(fd);
    }
    if ((result < 0) && !state_should_stop(context->state)) {
        state_set_degraded(context->state, selected, error);
    }
    return result;
}

void *capture_thread_main(void *argument)
{
    CaptureContext *context = argument;
    char error[SENSE_ERROR_MAX];

    while (!state_should_stop(context->state)) {
        error[0] = '\0';
        if (capture_once(context, error, sizeof(error)) == 0) {
            break;
        }
        if (!state_should_stop(context->state)) {
            fprintf(stderr, "capture degraded: %s\n", error);
            retry_delay(context->state);
        }
    }
    return NULL;
}
