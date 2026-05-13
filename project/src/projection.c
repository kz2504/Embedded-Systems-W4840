#include "projection.h"

#include <math.h>
#include <string.h>

#define J4_N 4
#define J4_ITERS 64
#define J4_EPS 1.0e-12

static double dot3(const double a[3], const double b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void cross3(const double a[3], const double b[3], double out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static double norm3(const double v[3])
{
    return sqrt(dot3(v, v));
}

static int normalize3(double v[3])
{
    double n = norm3(v);

    if (n < 1.0e-12) {
        return -1;
    }
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
    return 0;
}

static void mat3_vec(const double m[9], const double v[3], double out[3])
{
    for (int r = 0; r < 3; r++) {
        out[r] = m[3 * r + 0] * v[0] +
                 m[3 * r + 1] * v[1] +
                 m[3 * r + 2] * v[2];
    }
}

static void rotation_from_to(const double from_in[3], const double to_in[3],
                             double r[9])
{
    double from[3] = {from_in[0], from_in[1], from_in[2]};
    double to[3] = {to_in[0], to_in[1], to_in[2]};
    double v[3];
    double c;
    double s;
    double vx[9];

    projection_identity3(r);
    if (normalize3(from) < 0 || normalize3(to) < 0) {
        return;
    }

    c = dot3(from, to);
    if (c > 0.999999) {
        return;
    }

    cross3(from, to, v);
    s = norm3(v);
    if (s < 1.0e-12) {
        r[0] = -1.0;
        r[4] = 1.0;
        r[8] = -1.0;
        return;
    }

    vx[0] = 0.0;
    vx[1] = -v[2];
    vx[2] = v[1];
    vx[3] = v[2];
    vx[4] = 0.0;
    vx[5] = -v[0];
    vx[6] = -v[1];
    vx[7] = v[0];
    vx[8] = 0.0;

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            double vx2 = 0.0;
            for (int k = 0; k < 3; k++) {
                vx2 += vx[3 * row + k] * vx[3 * k + col];
            }
            r[3 * row + col] += vx[3 * row + col] +
                                vx2 * ((1.0 - c) / (s * s));
        }
    }
}

static void jacobi_smallest_eigenvector_4(const double a_in[J4_N][J4_N],
                                          double v_out[J4_N])
{
    double a[J4_N][J4_N];
    double v[J4_N][J4_N] = {{0.0}};

    memcpy(a, a_in, sizeof(a));
    for (int i = 0; i < J4_N; i++) {
        v[i][i] = 1.0;
    }

    for (int iter = 0; iter < J4_ITERS; iter++) {
        int p = 0;
        int q = 1;
        double max_off = fabs(a[p][q]);

        for (int r = 0; r < J4_N; r++) {
            for (int c = r + 1; c < J4_N; c++) {
                double off = fabs(a[r][c]);
                if (off > max_off) {
                    max_off = off;
                    p = r;
                    q = c;
                }
            }
        }

        if (max_off < J4_EPS) {
            break;
        }

        double app = a[p][p];
        double aqq = a[q][q];
        double apq = a[p][q];
        double tau = (aqq - app) / (2.0 * apq);
        double t = (tau >= 0.0 ? 1.0 : -1.0) /
                   (fabs(tau) + sqrt(1.0 + tau * tau));
        double c = 1.0 / sqrt(1.0 + t * t);
        double s = t * c;

        a[p][p] = app - t * apq;
        a[q][q] = aqq + t * apq;
        a[p][q] = 0.0;
        a[q][p] = 0.0;

        for (int k = 0; k < J4_N; k++) {
            if (k != p && k != q) {
                double akp = a[k][p];
                double akq = a[k][q];
                a[k][p] = c * akp - s * akq;
                a[p][k] = a[k][p];
                a[k][q] = s * akp + c * akq;
                a[q][k] = a[k][q];
            }
        }

        for (int k = 0; k < J4_N; k++) {
            double vkp = v[k][p];
            double vkq = v[k][q];
            v[k][p] = c * vkp - s * vkq;
            v[k][q] = s * vkp + c * vkq;
        }
    }

    int smallest = 0;
    for (int i = 1; i < J4_N; i++) {
        if (a[i][i] < a[smallest][smallest]) {
            smallest = i;
        }
    }
    for (int i = 0; i < J4_N; i++) {
        v_out[i] = v[i][smallest];
    }
}

void projection_identity3(double r[9])
{
    memset(r, 0, 9 * sizeof(double));
    r[0] = 1.0;
    r[4] = 1.0;
    r[8] = 1.0;
}

double projection_distance3(const double a[3], const double b[3])
{
    double d[3] = {
        b[0] - a[0],
        b[1] - a[1],
        b[2] - a[2],
    };

    return norm3(d);
}

point2_t projection_normalize_pixel(const double k[9], point2_t pixel)
{
    point2_t out;

    out.x = (pixel.x - k[2]) / k[0];
    out.y = (pixel.y - k[5]) / k[4];
    return out;
}

void projection_make_left_camera(const double k[9], double p[12])
{
    static const double eye[9] = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    };
    static const double zero[3] = {0.0, 0.0, 0.0};

    projection_from_pose(k, eye, zero, p);
}

void projection_from_pose(const double k[9], const double r[9],
                          const double t[3], double p[12])
{
    double rt[12];

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            rt[4 * row + col] = r[3 * row + col];
        }
        rt[4 * row + 3] = t[row];
    }

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            p[4 * row + col] = 0.0;
            for (int kk = 0; kk < 3; kk++) {
                p[4 * row + col] += k[3 * row + kk] * rt[4 * kk + col];
            }
        }
    }
}

int projection_project(const double p[12], const double x[3],
                       point2_t *image_point)
{
    double h[4] = {x[0], x[1], x[2], 1.0};
    double u = 0.0;
    double v = 0.0;
    double w = 0.0;

    for (int i = 0; i < 4; i++) {
        u += p[i] * h[i];
        v += p[4 + i] * h[i];
        w += p[8 + i] * h[i];
    }

    if (fabs(w) < 1.0e-12) {
        return -1;
    }

    image_point->x = u / w;
    image_point->y = v / w;
    return 0;
}

void projection_apply_transform(const double raw[3], const double origin[3],
                                int origin_set, const double rotation[9],
                                double scale, double out[3])
{
    double rotated[3];

    projection_apply_transform_unscaled(raw, origin, origin_set, rotation,
                                        rotated);
    out[0] = scale * rotated[0];
    out[1] = scale * rotated[1];
    out[2] = -scale * rotated[2];
}

void projection_apply_transform_unscaled(const double raw[3],
                                         const double origin[3],
                                         int origin_set,
                                         const double rotation[9],
                                         double out[3])
{
    double shifted[3];

    shifted[0] = raw[0] - (origin_set ? origin[0] : 0.0);
    shifted[1] = raw[1] - (origin_set ? origin[1] : 0.0);
    shifted[2] = raw[2] - (origin_set ? origin[2] : 0.0);
    mat3_vec(rotation, shifted, out);
}

int projection_floor_rotation_from_points(const double p0[3],
                                          const double p1[3],
                                          const double p2[3],
                                          double rotation_out[9])
{
    double a[3];
    double b[3];
    double normal[3];
    const double up[3] = {0.0, 0.0, 1.0};

    for (int i = 0; i < 3; i++) {
        a[i] = p1[i] - p0[i];
        b[i] = p2[i] - p0[i];
    }
    cross3(a, b, normal);
    if (normalize3(normal) < 0) {
        return -1;
    }

    rotation_from_to(normal, up, rotation_out);
    return 0;
}

int projection_triangulate_normalized(const point2_t *left,
                                      const point2_t *right,
                                      const double r[9],
                                      const double t[3],
                                      double x_out[3])
{
    double rows[4][4] = {
        {-1.0, 0.0, left->x, 0.0},
        {0.0, -1.0, left->y, 0.0},
        {
            right->x * r[6] - r[0],
            right->x * r[7] - r[1],
            right->x * r[8] - r[2],
            right->x * t[2] - t[0],
        },
        {
            right->y * r[6] - r[3],
            right->y * r[7] - r[4],
            right->y * r[8] - r[5],
            right->y * t[2] - t[1],
        },
    };
    double ata[4][4] = {{0.0}};
    double h[4];

    for (int row = 0; row < 4; row++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                ata[i][j] += rows[row][i] * rows[row][j];
            }
        }
    }

    jacobi_smallest_eigenvector_4(ata, h);
    if (fabs(h[3]) < 1.0e-12) {
        return -1;
    }

    x_out[0] = h[0] / h[3];
    x_out[1] = h[1] / h[3];
    x_out[2] = h[2] / h[3];
    return 0;
}

int projection_triangulate(const double p_left[12], const double p_right[12],
                           const point2_t *left, const point2_t *right,
                           double x_out[3])
{
    double rows[4][4];
    double ata[4][4] = {{0.0}};
    double h[4];

    for (int c = 0; c < 4; c++) {
        rows[0][c] = left->x * p_left[8 + c] - p_left[c];
        rows[1][c] = left->y * p_left[8 + c] - p_left[4 + c];
        rows[2][c] = right->x * p_right[8 + c] - p_right[c];
        rows[3][c] = right->y * p_right[8 + c] - p_right[4 + c];
    }

    for (int row = 0; row < 4; row++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                ata[i][j] += rows[row][i] * rows[row][j];
            }
        }
    }

    jacobi_smallest_eigenvector_4(ata, h);
    if (fabs(h[3]) < 1.0e-12) {
        return -1;
    }

    x_out[0] = h[0] / h[3];
    x_out[1] = h[1] / h[3];
    x_out[2] = h[2] / h[3];
    return 0;
}
