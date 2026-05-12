#include "essential.h"

#include "projection.h"

#include <math.h>
#include <string.h>

#define E_N 3
#define E_JACOBI_ITERS 64
#define E_JACOBI_EPS 1.0e-12

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

static void normalize3(double v[3])
{
    double n = sqrt(dot3(v, v));

    if (n > 0.0) {
        v[0] /= n;
        v[1] /= n;
        v[2] /= n;
    }
}

static double det3(const double m[9])
{
    return m[0] * (m[4] * m[8] - m[5] * m[7]) -
           m[1] * (m[3] * m[8] - m[5] * m[6]) +
           m[2] * (m[3] * m[7] - m[4] * m[6]);
}

static void mat3_transpose(const double in[9], double out[9])
{
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out[3 * r + c] = in[3 * c + r];
        }
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

static void jacobi_eigen_symmetric3(const double a_in[9], double eval[3],
                                    double evec[9])
{
    double a[9];

    memcpy(a, a_in, sizeof(a));
    memset(evec, 0, 9 * sizeof(double));
    evec[0] = 1.0;
    evec[4] = 1.0;
    evec[8] = 1.0;

    for (int iter = 0; iter < E_JACOBI_ITERS; iter++) {
        int p = 0;
        int q = 1;
        double max_off = fabs(a[1]);

        if (fabs(a[2]) > max_off) {
            max_off = fabs(a[2]);
            p = 0;
            q = 2;
        }
        if (fabs(a[5]) > max_off) {
            max_off = fabs(a[5]);
            p = 1;
            q = 2;
        }
        if (max_off < E_JACOBI_EPS) {
            break;
        }

        double app = a[3 * p + p];
        double aqq = a[3 * q + q];
        double apq = a[3 * p + q];
        double tau = (aqq - app) / (2.0 * apq);
        double t = (tau >= 0.0 ? 1.0 : -1.0) /
                   (fabs(tau) + sqrt(1.0 + tau * tau));
        double c = 1.0 / sqrt(1.0 + t * t);
        double s = t * c;

        a[3 * p + p] = app - t * apq;
        a[3 * q + q] = aqq + t * apq;
        a[3 * p + q] = 0.0;
        a[3 * q + p] = 0.0;

        for (int k = 0; k < 3; k++) {
            if (k != p && k != q) {
                double akp = a[3 * k + p];
                double akq = a[3 * k + q];
                a[3 * k + p] = c * akp - s * akq;
                a[3 * p + k] = a[3 * k + p];
                a[3 * k + q] = s * akp + c * akq;
                a[3 * q + k] = a[3 * k + q];
            }
        }

        for (int k = 0; k < 3; k++) {
            double vkp = evec[3 * k + p];
            double vkq = evec[3 * k + q];
            evec[3 * k + p] = c * vkp - s * vkq;
            evec[3 * k + q] = s * vkp + c * vkq;
        }
    }

    eval[0] = a[0];
    eval[1] = a[4];
    eval[2] = a[8];

    for (int i = 0; i < 2; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (eval[j] > eval[i]) {
                double ev = eval[i];
                eval[i] = eval[j];
                eval[j] = ev;
                for (int r = 0; r < 3; r++) {
                    double vv = evec[3 * r + i];
                    evec[3 * r + i] = evec[3 * r + j];
                    evec[3 * r + j] = vv;
                }
            }
        }
    }
}

static void svd3(const double e[9], double u[9], double v[9])
{
    double et[9];
    double ete[9];
    double eval[3];
    double col0[3];
    double col1[3];
    double col2[3];

    mat3_transpose(e, et);
    mat3_mul(et, e, ete);
    jacobi_eigen_symmetric3(ete, eval, v);

    memset(u, 0, 9 * sizeof(double));
    for (int col = 0; col < 3; col++) {
        double sigma = eval[col] > 0.0 ? sqrt(eval[col]) : 0.0;

        if (sigma > 1.0e-12) {
            for (int row = 0; row < 3; row++) {
                for (int k = 0; k < 3; k++) {
                    u[3 * row + col] += e[3 * row + k] * v[3 * k + col];
                }
                u[3 * row + col] /= sigma;
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        col0[i] = u[3 * i + 0];
        col1[i] = u[3 * i + 1];
    }
    normalize3(col0);
    double dot01 = dot3(col0, col1);
    for (int i = 0; i < 3; i++) {
        col1[i] -= dot01 * col0[i];
    }
    normalize3(col1);
    cross3(col0, col1, col2);
    normalize3(col2);

    for (int i = 0; i < 3; i++) {
        u[3 * i + 0] = col0[i];
        u[3 * i + 1] = col1[i];
        u[3 * i + 2] = col2[i];
    }

    if (det3(u) < 0.0) {
        for (int i = 0; i < 3; i++) {
            u[3 * i + 2] = -u[3 * i + 2];
        }
    }
    if (det3(v) < 0.0) {
        for (int i = 0; i < 3; i++) {
            v[3 * i + 2] = -v[3 * i + 2];
        }
    }
}

void essential_from_fundamental(const double k_left[9],
                                const double k_right[9],
                                const double f[9],
                                double e_out[9])
{
    double k_right_t[9];
    double tmp[9];

    mat3_transpose(k_right, k_right_t);
    mat3_mul(k_right_t, f, tmp);
    mat3_mul(tmp, k_left, e_out);
}

static void make_candidate(const double u[9], const double v[9],
                           const double w[9], const double t[3],
                           camera_pose_t *pose)
{
    double vt[9];
    double tmp[9];

    mat3_transpose(v, vt);
    mat3_mul(u, w, tmp);
    mat3_mul(tmp, vt, pose->r);

    if (det3(pose->r) < 0.0) {
        for (int i = 0; i < 9; i++) {
            pose->r[i] = -pose->r[i];
        }
    }

    pose->t[0] = t[0];
    pose->t[1] = t[1];
    pose->t[2] = t[2];
}

static int chirality_score(const camera_pose_t *pose, const point2_t *left,
                           const point2_t *right, size_t count)
{
    int score = 0;

    for (size_t i = 0; i < count; i++) {
        double x[3];
        double z_right;

        if (projection_triangulate_normalized(&left[i], &right[i],
                                              pose->r, pose->t, x) < 0) {
            continue;
        }

        z_right = pose->r[6] * x[0] + pose->r[7] * x[1] +
                  pose->r[8] * x[2] + pose->t[2];
        if (x[2] > 0.0 && z_right > 0.0) {
            score++;
        }
    }

    return score;
}

int essential_recover_pose(const double e[9], const point2_t *left_norm,
                           const point2_t *right_norm, size_t count,
                           camera_pose_t *pose_out)
{
    static const double w[9] = {
        0.0, -1.0, 0.0,
        1.0,  0.0, 0.0,
        0.0,  0.0, 1.0,
    };
    static const double wt[9] = {
        0.0, 1.0, 0.0,
       -1.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
    };
    double u[9];
    double v[9];
    double t[3];
    camera_pose_t candidates[4];
    int best = -1;
    int best_score = -1;

    if (!e || !left_norm || !right_norm || !pose_out || count == 0) {
        return -1;
    }

    svd3(e, u, v);
    t[0] = u[2];
    t[1] = u[5];
    t[2] = u[8];
    normalize3(t);

    make_candidate(u, v, w, t, &candidates[0]);
    t[0] = -t[0];
    t[1] = -t[1];
    t[2] = -t[2];
    make_candidate(u, v, w, t, &candidates[1]);
    t[0] = -t[0];
    t[1] = -t[1];
    t[2] = -t[2];
    make_candidate(u, v, wt, t, &candidates[2]);
    t[0] = -t[0];
    t[1] = -t[1];
    t[2] = -t[2];
    make_candidate(u, v, wt, t, &candidates[3]);

    for (int i = 0; i < 4; i++) {
        int score = chirality_score(&candidates[i], left_norm, right_norm,
                                    count);
        if (score > best_score) {
            best_score = score;
            best = i;
        }
    }

    if (best < 0) {
        return -1;
    }

    *pose_out = candidates[best];
    return best_score > 0 ? 0 : -1;
}

void essential_projection_matrix(const double k[9],
                                 const camera_pose_t *pose,
                                 double p_out[12])
{
    projection_from_pose(k, pose->r, pose->t, p_out);
}
