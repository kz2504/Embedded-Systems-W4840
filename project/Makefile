TARGET := stereo
HOST_TARGET := serial_frame_decode

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CPPFLAGS ?=
CPPFLAGS += -Iinclude
CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDLIBS ?= -lm

.PHONY: all run host-decode clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

host-decode: $(HOST_TARGET)

$(HOST_TARGET): tools/serial_frame_decode.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -rf build $(TARGET) $(HOST_TARGET)
