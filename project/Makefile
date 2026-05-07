TARGETS := ov7670_i2c_probe imgproc_scan_control imgproc_capture_pgm
ARGS ?=
USB_DIR ?= /mnt/usb

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -O2 -Wall -Wextra -std=c11

.PHONY: all run scan-control capture capture-usb clean

all: $(TARGETS)

ov7670_i2c_probe: ov7670_i2c_probe.c
	$(CC) $(CFLAGS) -o $@ $<

imgproc_scan_control: imgproc_scan_control.c
	$(CC) $(CFLAGS) -o $@ $<

imgproc_capture_pgm: imgproc_capture_pgm.c
	$(CC) $(CFLAGS) -o $@ $<

run: ov7670_i2c_probe
	./ov7670_i2c_probe $(ARGS)

scan-control: imgproc_scan_control
	./imgproc_scan_control

capture: imgproc_capture_pgm
	./imgproc_capture_pgm $(ARGS)

capture-usb: imgproc_capture_pgm
	./imgproc_capture_pgm $(USB_DIR)/frames

clean:
	rm -f $(TARGETS)
