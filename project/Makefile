TARGETS := ov7670_i2c_probe imgproc_scan_control imgproc_capture_pgm
ARGS ?=
CAM ?= A

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -O2 -Wall -Wextra -std=c11

.PHONY: all run scan-control capture serial-capture host-decode clean

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
	./imgproc_capture_pgm $(CAM) $(ARGS)

serial-capture: imgproc_capture_pgm
	./imgproc_capture_pgm $(CAM) --serial

host-decode: serial_frame_decode.c
	$(CC) $(CFLAGS) -o serial_frame_decode serial_frame_decode.c

clean:
	rm -f $(TARGETS) serial_frame_decode
