#ifndef ESSENTIAL_H
#define ESSENTIAL_H

#include "fundamental.h"

#include <stddef.h>

typedef struct {
    double r[9];
    double t[3];
} camera_pose_t;

void essential_from_fundamental(const double k_left[9],
                                const double k_right[9],
                                const double f[9],
                                double e_out[9]);
int essential_recover_pose(const double e[9], const point2_t *left_norm,
                           const point2_t *right_norm, size_t count,
                           camera_pose_t *pose_out);
void essential_projection_matrix(const double k[9],
                                 const camera_pose_t *pose,
                                 double p_out[12]);

#endif
