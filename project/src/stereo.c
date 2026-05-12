#include "stereo.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_AGC 0
#define DEFAULT_AEC 1
#define DEFAULT_EXPOSURE 0x0100u
#define DEFAULT_GAIN_A 0x40u
#define DEFAULT_GAIN_B 0x60u

void camera_settings_default(camera_settings_t *settings)
{
    camera_settings_default_for(CAMERA_A, settings);
}

void camera_settings_default_for(camera_select_t camera,
                                 camera_settings_t *settings)
{
    settings->gain = camera == CAMERA_B ? DEFAULT_GAIN_B : DEFAULT_GAIN_A;
    settings->exposure = DEFAULT_EXPOSURE;
    settings->gain_ceiling = 0;
    settings->agc = DEFAULT_AGC;
    settings->aec = DEFAULT_AEC;
    settings->exposure_set = 1;
    settings->gain_ceiling_set = 0;
}

int parse_unsigned_arg(const char *text, unsigned max, unsigned *value_out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || *end || value > max) {
        return -1;
    }

    *value_out = (unsigned)value;
    return 0;
}

int parse_bool_arg(const char *text, int *value_out)
{
    if (strcmp(text, "1") == 0 || strcmp(text, "on") == 0 ||
        strcmp(text, "ON") == 0 || strcmp(text, "true") == 0 ||
        strcmp(text, "TRUE") == 0 || strcmp(text, "yes") == 0 ||
        strcmp(text, "YES") == 0) {
        *value_out = 1;
        return 0;
    }

    if (strcmp(text, "0") == 0 || strcmp(text, "off") == 0 ||
        strcmp(text, "OFF") == 0 || strcmp(text, "false") == 0 ||
        strcmp(text, "FALSE") == 0 || strcmp(text, "no") == 0 ||
        strcmp(text, "NO") == 0) {
        *value_out = 0;
        return 0;
    }

    return -1;
}

int camera_parse_select(const char *text, camera_select_t *camera)
{
    if (strcmp(text, "A") == 0 || strcmp(text, "a") == 0 ||
        strcmp(text, "0") == 0) {
        *camera = CAMERA_A;
        return 0;
    }
    if (strcmp(text, "B") == 0 || strcmp(text, "b") == 0 ||
        strcmp(text, "1") == 0) {
        *camera = CAMERA_B;
        return 0;
    }
    if (strcmp(text, "both") == 0 || strcmp(text, "BOTH") == 0 ||
        strcmp(text, "all") == 0 || strcmp(text, "ALL") == 0) {
        *camera = CAMERA_BOTH;
        return 0;
    }

    return -1;
}

int camera_parse_single(const char *text, camera_select_t *camera)
{
    if (camera_parse_select(text, camera) < 0) {
        return -1;
    }
    return *camera == CAMERA_A || *camera == CAMERA_B ? 0 : -1;
}

char camera_letter(camera_select_t camera)
{
    return camera == CAMERA_B ? 'B' : 'A';
}

const char *camera_label(camera_select_t camera)
{
    return camera == CAMERA_B ? "camera B" : "camera A";
}
