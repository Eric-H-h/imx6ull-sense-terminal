CROSS_COMPILE ?=

ifeq ($(origin CC), default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11 -O2 -pthread
LDLIBS ?= -pthread
TARGET := imx6ull-sense

SRCS := main.c config.c state.c capture_v4l2.c http_server.c
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)

