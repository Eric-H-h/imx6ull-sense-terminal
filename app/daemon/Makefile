CROSS_COMPILE ?=

ifeq ($(origin CC), default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11 -O2 -pthread
STANDALONE_CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11 -O2
LDLIBS ?= -pthread -ljpeg
TARGET := imx6ull-sense
JPEG_DECODER_TEST := build/test-jpeg-decoder
MOTION_DETECTOR_TEST := build/test-motion-detector
MOTION_EVENT_GATE_TEST := build/test-motion-event-gate
EVENT_LOG_TEST := build/test-event-log
MOTION_PIPELINE_TEST := build/test-motion-pipeline
MOTION_WORKER_TEST := build/test-motion-worker
CONFIG_TEST := build/test-config
STATUS_JSON_TEST := build/test-status-json
STATE_TEST := build/test-state
JPEG_BENCH_HOST := build/jpeg-gray-bench-host
JPEG_BENCH_ARM := build/jpeg-gray-bench-arm
HOST_CC ?= gcc
ARM_CC ?=
ARM_JPEG_ROOT ?=

SRCS := main.c config.c state.c capture_v4l2.c http_server.c \
	jpeg_decoder.c motion_detector.c motion_event_gate.c event_log.c \
	motion_pipeline.c motion_worker.c status_json.c
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test verify test-jpeg-decoder test-motion-detector \
	test-motion-event-gate test-event-log test-motion-pipeline \
	test-motion-worker test-config test-status-json test-state \
	jpeg-gray-bench-host jpeg-gray-bench-arm

all: $(TARGET)

test: test-jpeg-decoder test-motion-detector test-motion-event-gate \
	test-event-log test-motion-pipeline test-motion-worker test-config \
	test-status-json test-state

verify:
	$(MAKE) clean
	$(MAKE) all
	$(MAKE) test

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(CONFIG_TEST): tests/test_config.c config.c sense.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tests/test_config.c config.c -lm

test-config: $(CONFIG_TEST)
	./$(CONFIG_TEST)

$(STATUS_JSON_TEST): tests/test_status_json.c status_json.c \
		status_json.h sense.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tests/test_status_json.c status_json.c

test-status-json: $(STATUS_JSON_TEST)
	./$(STATUS_JSON_TEST)

$(STATE_TEST): tests/test_state.c state.c sense.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -I. -o $@ \
		tests/test_state.c state.c

test-state: $(STATE_TEST)
	./$(STATE_TEST)

$(JPEG_DECODER_TEST): tests/test_jpeg_decoder.c jpeg_decoder.c \
		jpeg_decoder.h Makefile
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I. -o $@ \
		tests/test_jpeg_decoder.c jpeg_decoder.c -ljpeg

test-jpeg-decoder: $(JPEG_DECODER_TEST)
	./$(JPEG_DECODER_TEST)

$(MOTION_DETECTOR_TEST): tests/test_motion_detector.c motion_detector.c \
		motion_detector.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tests/test_motion_detector.c motion_detector.c

test-motion-detector: $(MOTION_DETECTOR_TEST)
	./$(MOTION_DETECTOR_TEST)

$(MOTION_EVENT_GATE_TEST): tests/test_motion_event_gate.c \
		motion_event_gate.c motion_event_gate.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tests/test_motion_event_gate.c motion_event_gate.c

test-motion-event-gate: $(MOTION_EVENT_GATE_TEST)
	./$(MOTION_EVENT_GATE_TEST)

$(EVENT_LOG_TEST): tests/test_event_log.c event_log.c event_log.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tests/test_event_log.c event_log.c

test-event-log: $(EVENT_LOG_TEST)
	./$(EVENT_LOG_TEST)

$(MOTION_PIPELINE_TEST): tests/test_motion_pipeline.c motion_pipeline.c \
		motion_pipeline.h jpeg_decoder.c jpeg_decoder.h \
		motion_detector.c motion_detector.h motion_event_gate.c \
		motion_event_gate.h event_log.c event_log.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tests/test_motion_pipeline.c motion_pipeline.c jpeg_decoder.c \
		motion_detector.c motion_event_gate.c event_log.c -ljpeg

test-motion-pipeline: $(MOTION_PIPELINE_TEST)
	./$(MOTION_PIPELINE_TEST)

$(MOTION_WORKER_TEST): tests/test_motion_worker.c motion_worker.c sense.h \
		state.c motion_pipeline.c motion_pipeline.h jpeg_decoder.c \
		jpeg_decoder.h motion_detector.c motion_detector.h \
		motion_event_gate.c motion_event_gate.h event_log.c event_log.h \
		Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(CFLAGS) -I. -o $@ \
		tests/test_motion_worker.c motion_worker.c state.c \
		motion_pipeline.c jpeg_decoder.c motion_detector.c \
		motion_event_gate.c event_log.c -ljpeg

test-motion-worker: $(MOTION_WORKER_TEST)
	./$(MOTION_WORKER_TEST)

$(JPEG_BENCH_HOST): tools/jpeg_gray_bench.c jpeg_decoder.c \
		jpeg_decoder.h Makefile
	mkdir -p $(dir $@)
	$(HOST_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. -o $@ \
		tools/jpeg_gray_bench.c jpeg_decoder.c -ljpeg

jpeg-gray-bench-host: $(JPEG_BENCH_HOST)

jpeg-gray-bench-arm:
	@test -n "$(ARM_CC)" || \
		(echo "ARM_CC is required" >&2; exit 2)
	@test -n "$(ARM_JPEG_ROOT)" || \
		(echo "ARM_JPEG_ROOT is required" >&2; exit 2)
	@test -f "$(ARM_JPEG_ROOT)/usr/include/jpeglib.h" || \
		(echo "jpeglib.h not found under ARM_JPEG_ROOT" >&2; exit 2)
	mkdir -p $(dir $(JPEG_BENCH_ARM))
	$(ARM_CC) $(CPPFLAGS) $(STANDALONE_CFLAGS) -I. \
		-I$(ARM_JPEG_ROOT)/usr/include \
		-I$(ARM_JPEG_ROOT)/usr/include/arm-linux-gnueabihf \
		-L$(ARM_JPEG_ROOT)/usr/lib/arm-linux-gnueabihf \
		-Wl,-rpath-link,$(ARM_JPEG_ROOT)/usr/lib/arm-linux-gnueabihf \
		-o $(JPEG_BENCH_ARM) tools/jpeg_gray_bench.c jpeg_decoder.c -ljpeg

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS) $(JPEG_DECODER_TEST) \
		$(MOTION_DETECTOR_TEST) $(MOTION_EVENT_GATE_TEST) \
		$(EVENT_LOG_TEST) $(MOTION_PIPELINE_TEST) \
		$(MOTION_WORKER_TEST) $(CONFIG_TEST) $(STATUS_JSON_TEST) \
		$(STATE_TEST) $(JPEG_BENCH_HOST) $(JPEG_BENCH_ARM)

-include $(DEPS)
