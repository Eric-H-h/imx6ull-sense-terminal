CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
TARGET := imx6ull-sense

SRCS := main.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)

