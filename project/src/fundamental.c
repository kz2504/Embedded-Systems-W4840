#include "fundamental.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define F_N 9
#define JACOBI_ITERS 128
#define JACOBI_EPS 1.0e-12

static void jacobi_smallest_eigenvector_9(const double a_in[F_N][F_N],
                                          double v_out[F_N])
{
    double a[F_N][F_N];
    double v[F_N][F_N] = {{0.0}};

    memcpy(a, a_in, sizeof(a));
    for (int i = 0; i < F_N; i++) {
        v[i][i] = 1.0;
    }

    for (int iter = 0; iter < JACOBI_ITERS; iter++) {
        int p = 0;
        int q = 1;
        double max_off = fabs(a[p][q]);

        for (int r = 0; r < F_N; r++) {
            for (int c = r + 1; c < F_N; c++) {
                double off = fabs(a[r][c]);
                if (off > max_off) {
                    max_off = off;
                    p = r;
                    q = c;
                }
            }
        }

        if (max_off < JACOBI_EPS) {
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

        for (int k = 0; k < F_N; k++) {
            if (k != p && k != q) {
                double akp = a[k][p];
                double akq = a[k][q];
                a[k][p] = c * akp - s * akq;
                a[p][k] = a[k][p];
                a[k][q] = s * akp + c * akq;
                a[q][k] = a[k][q];
            }
        }

        for (int k = 0; k < F_N; k++) {
            double vkp = v[k][p];
            double vkq = v[k][q];
            v[k][p] = c * vkp - s * vkq;
            v[k][q] = s * vkp + c * vkq;
        }
    }

    int smallest = 0;
    for (int i = 1; i < F_N; i++) {
        if (a[i][i] < a[smallest][smallest]) {
            smallest = i;
        }
    }

    for (int i = 0; i < F_N; i++) {
        v_out[i] = v[i][smallest];
    }
}

static void normalize_points(const point2_t *in, point2_t *out, size_t count,
                             double t[9])
{
    double mx = 0.0;
    double my = 0.0;
    double avg_dist = 0.0;

    for (size_t i = 0; i < count; i++) {
        mx += in[i].x;
        my += in[i].y;
    }
    mx /= (double)count;
    my /= (double)count;

    for (size_t i = 0; i < count; i++) {
        double dx = in[i].x - mx;
        double dy = in[i].y - my;
        avg_dist += sqrt(dx * dx + dy * dy);
    }
    avg_dist /= (double)count;

    double s = avg_dist > 0.0 ? sqrt(2.0) / avg_dist : 1.0;

    t[0] = s;
    t[1] = 0.0;
    t[2] = -s * mx;
    t[3] = 0.0;
    t[4] = s;
    t[5] = -s * my;
    t[6] = 0.0;
    t[7] = 0.0;
    t[8] = 1.0;

    for (size_t i = 0; i < count; i++) {
        out[i].x = s * (in[i].x - mx);
        out[i].y = s * (in[i].y - my);
    }
}

static void mat3_mul(const double a[9], const double b[9], double out[9])
{
    double tmp[9];

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            tmp[3 * r + c] = 0.0;
            for (int k = 0; k < 3; k++) {
                tmp[3 * r + c] += a[3 * r + k] * b[3 * k + c];
            }
        }
    }
    memcpy(out, tmp, sizeof(tmp));
}

static void mat3_transpose(const double in[9], double out[9])
{
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out[3 * r + c] = in[3 * c + r];
        }
    }
}

int fundamental_estimate(const point2_t *left, const point2_t *right,
                         size_t count, double f_out[9])
{
    double ata[F_N][F_N] = {{0.0}};
    double f_norm[9];
    double t_left[9];
    double t_right[9];
    double t_right_t[9];
    double tmp[9];
    point2_t *left_norm;
    point2_t *right_norm;

    if (!left || !right || !f_out || count < 8) {
        return -1;
    }

    left_norm = malloc(count * sizeof(*left_norm));
    right_norm = malloc(count * sizeof(*right_norm));
    if (!left_norm || !right_norm) {
        free(left_norm);
        free(right_norm);
        return -1;
    }

    normalize_points(left, left_norm, count, t_left);
    normalize_points(right, right_norm, count, t_right);

    for (size_t i = 0; i < count; i++) {
        double x1 = left_norm[i].x;
        double y1 = left_norm[i].y;
        double x2 = right_norm[i].x;
        double y2 = right_norm[i].y;
        double row[F_N] = {
            x2 * x1, x2 * y1, x2,
            y2 * x1, y2 * y1, y2,
            x1,      y1,      1.0,
        };

        for (int r = 0; r < F_N; r++) {
            for (int c = 0; c < F_N; c++) {
                ata[r][c] += row[r] * row[c];
            }
        }
    }

    jacobi_smallest_eigenvector_9(ata, f_norm);

    mat3_transpose(t_right, t_right_t);
    mat3_mul(t_right_t, f_norm, tmp);
    mat3_mul(tmp, t_left, f_out);

    double norm = 0.0;
    for (int i = 0; i < 9; i++) {
        norm += f_out[i] * f_out[i];
    }
    norm = sqrt(norm);
    if (norm > 0.0) {
        for (int i = 0; i < 9; i++) {
            f_out[i] /= norm;
        }
    }

    free(left_norm);
    free(right_norm);
    return 0;
}
