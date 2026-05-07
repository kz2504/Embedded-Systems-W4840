TARGETS := ov7670_i2c_probe imgproc_scan_control
ARGS ?=

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -O2 -Wall -Wextra -std=c11

.PHONY: all run clean

all: $(TARGETS)

ov7670_i2c_probe: ov7670_i2c_probe.c
	$(CC) $(CFLAGS) -o $@ $<

imgproc_scan_control: imgproc_scan_control.c
	$(CC) $(CFLAGS) -o $@ $<

run: ov7670_i2c_probe
	./ov7670_i2c_probe $(ARGS)

scan-control: imgproc_scan_control
	./imgproc_scan_control

clean:
	rm -f $(TARGETS)
