#ifndef PROJECTION_H
#define PROJECTION_H

#include "fundamental.h"

void projection_identity3(double r[9]);
double projection_distance3(const double a[3], const double b[3]);
point2_t projection_normalize_pixel(const double k[9], point2_t pixel);
void projection_make_left_camera(const double k[9], double p[12]);
void projection_from_pose(const double k[9], const double r[9],
                          const double t[3], double p[12]);
int projection_project(const double p[12], const double x[3],
                       point2_t *image_point);
void projection_apply_transform(const double raw[3], const double origin[3],
                                int origin_set, const double rotation[9],
                                double scale, double out[3]);
void projection_apply_transform_unscaled(const double raw[3],
                                         const double origin[3],
                                         int origin_set,
                                         const double rotation[9],
                                         double out[3]);
int projection_floor_rotation_from_points(const double p0[3],
                                          const double p1[3],
                                          const double p2[3],
                                          double rotation_out[9]);
int projection_triangulate_normalized(const point2_t *left,
                                      const point2_t *right,
                                      const double r[9],
                                      const double t[3],
                                      double x_out[3]);
int projection_triangulate(const double p_left[12], const double p_right[12],
                           const point2_t *left, const point2_t *right,
                           double x_out[3]);

#endif
