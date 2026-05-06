TARGET := ov7670_i2c_probe
OLD_TARGETS := pio_gpio_test
ARGS ?=

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -O2 -Wall -Wextra -std=c11

.PHONY: all run clean

all: $(TARGET)

$(TARGET): ov7670_i2c_probe.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET) $(OLD_TARGETS)
