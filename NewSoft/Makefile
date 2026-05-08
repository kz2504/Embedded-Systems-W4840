TARGETS := ov7670_i2c_probe imgproc_scan_control imgproc_capture_pgm imgproc_moments_30fps

ARGS ?=
CAM ?= A

THRESH ?= 180
THRESH_A ?= $(THRESH)
THRESH_B ?= $(THRESH)

FOCAL_X ?= 500
FOCAL_Y ?= 500
MATCH_TARGET ?= 30

BOARD_COLS ?= 7
BOARD_ROWS ?= 6
SQUARE_SIZE ?= 1.0

CALIB_A_DIR ?= calib_A
CALIB_B_DIR ?= calib_B

ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CFLAGS ?= -O2 -Wall -Wextra -std=c11
LDLIBS ?= -lm

.PHONY: all run scan-control capture serial-capture moments \
        calib-dirs capture-calib-a capture-calib-b \
        calibrate-a calibrate-b calibrate-stereo \
        host-decode clean

all: $(TARGETS)

ov7670_i2c_probe: ov7670_i2c_probe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

imgproc_scan_control: imgproc_scan_control.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

imgproc_capture_pgm: imgproc_capture_pgm.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

imgproc_moments_30fps: imgproc_moments_30fps.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: ov7670_i2c_probe
	./ov7670_i2c_probe $(ARGS)

scan-control: imgproc_scan_control
	./imgproc_scan_control

capture: imgproc_capture_pgm
	./imgproc_capture_pgm $(CAM) $(ARGS)

serial-capture: imgproc_capture_pgm
	./imgproc_capture_pgm $(CAM) --serial

moments: imgproc_moments_30fps
	./imgproc_moments_30fps $(THRESH_A) $(THRESH_B) $(FOCAL_X) $(FOCAL_Y) $(MATCH_TARGET)

calib-dirs:
	mkdir -p $(CALIB_A_DIR) $(CALIB_B_DIR)

capture-calib-a: imgproc_capture_pgm calib-dirs
	./imgproc_capture_pgm A $(CALIB_A_DIR)

capture-calib-b: imgproc_capture_pgm calib-dirs
	./imgproc_capture_pgm B $(CALIB_B_DIR)

calibrate-a:
	python3 calibrate_intrinsics.py --images "$(CALIB_A_DIR)/*.pgm" --board $(BOARD_COLS) $(BOARD_ROWS) --square $(SQUARE_SIZE) --out camera_A_intrinsics.npz

calibrate-b:
	python3 calibrate_intrinsics.py --images "$(CALIB_B_DIR)/*.pgm" --board $(BOARD_COLS) $(BOARD_ROWS) --square $(SQUARE_SIZE) --out camera_B_intrinsics.npz

calibrate-stereo:
	python3 estimate_stereo_from_matches.py --matches stereo_matches.csv --intrinsics-a camera_A_intrinsics.npz --intrinsics-b camera_B_intrinsics.npz --out stereo_model.npz

host-decode: serial_frame_decode.c
	$(CC) $(CFLAGS) -o serial_frame_decode serial_frame_decode.c $(LDLIBS)

clean:
	rm -f $(TARGETS) serial_frame_decode

