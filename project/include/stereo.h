#ifndef STEREO_H
#define STEREO_H

#include <stdint.h>

#define DE1_LW_BRIDGE_BASE 0xFF200000u
#define DE1_LW_BRIDGE_SPAN 0x1000u

#define FRAME_WIDTH 640u
#define FRAME_HEIGHT 480u
#define FRAME_PIXELS (FRAME_WIDTH * FRAME_HEIGHT)
#define FRAME_WORDS (FRAME_PIXELS / 4u)

typedef enum {
    CAMERA_A = 1u << 0,
    CAMERA_B = 1u << 1,
    CAMERA_BOTH = CAMERA_A | CAMERA_B,
} camera_select_t;

typedef struct {
    unsigned gain;
    unsigned exposure;
    unsigned gain_ceiling;
    int agc;
    int aec;
    int exposure_set;
    int gain_ceiling_set;
} camera_settings_t;

typedef struct {
    uint32_t area;
    uint32_t u;
    uint32_t v;
    double cx;
    double cy;
} camera_moments_t;

void camera_settings_default(camera_settings_t *settings);
void camera_settings_default_for(camera_select_t camera,
                                 camera_settings_t *settings);
int camera_parse_select(const char *text, camera_select_t *camera);
int camera_parse_single(const char *text, camera_select_t *camera);
char camera_letter(camera_select_t camera);
const char *camera_label(camera_select_t camera);
int parse_unsigned_arg(const char *text, unsigned max, unsigned *value_out);
int parse_bool_arg(const char *text, int *value_out);

#endif
