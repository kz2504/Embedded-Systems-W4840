#ifndef FUNDAMENTAL_H
#define FUNDAMENTAL_H

#include <stddef.h>

typedef struct {
    double x;
    double y;
} point2_t;

int fundamental_estimate(const point2_t *left, const point2_t *right,
                         size_t count, double f_out[9]);

#endif
