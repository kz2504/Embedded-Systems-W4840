#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include "stereo.h"

#include <stddef.h>
#include <stdio.h>

int camera_capture_frame(camera_select_t camera, unsigned char *pixels);
int camera_save_pgm(const char *path, const unsigned char *pixels);
int camera_capture_save(camera_select_t camera, const char *out_dir,
                        char *path, size_t path_len);
int camera_capture_serial(camera_select_t camera, FILE *out);
int camera_set_thresholds(unsigned threshold_a, unsigned threshold_b);
int camera_read_moment_pair(camera_moments_t *moments_a,
                            camera_moments_t *moments_b,
                            unsigned min_area, int *valid);
int camera_debug_stream(unsigned print_every);
int camera_print_status(void);

#endif
