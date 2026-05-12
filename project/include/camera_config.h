#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include "stereo.h"

#define CAMERA_GAIN_MAX 0x3ffu
#define CAMERA_EXPOSURE_MAX 0xffffu
#define CAMERA_REG8_MAX 0xffu

int camera_config_parse_setting(const char *text, camera_settings_t *settings);
int camera_configure(camera_select_t cameras,
                     const camera_settings_t *settings);

#endif
