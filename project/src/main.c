#define _DEFAULT_SOURCE

#include "camera_capture.h"
#include "camera_config.h"
#include "essential.h"
#include "fundamental.h"
#include "projection.h"
#include "stereo.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#define CALIBRATION_FILE "stereo_projection.txt"
#define MAX_CORRESPONDENCES 128
#define MIN_FUNDAMENTAL_POINTS 8
#define LINE_LEN 256
#define MAX_ARGS 24
#define DEFAULT_MIN_AREA 10u
#define DEFAULT_THRESHOLD 0x0au

static const double K_A[9] = {
    879.693451, 0.0,        365.717241,
    0.0,        879.693451, 241.590062,
    0.0,        0.0,        1.0,
};

static const double K_B[9] = {
    900.939218, 0.0,        307.065820,
    0.0,        900.939218, 218.219419,
    0.0,        0.0,        1.0,
};

typedef struct {
    double p_a[12];
    double p_b[12];
    int valid;
} calibration_t;

typedef struct {
    unsigned threshold_a;
    unsigned threshold_b;
    unsigned min_area;
    calibration_t calibration;
    double origin[3];
    int origin_set;
    double floor_rot[9];
    int floor_set;
    double floor_points[3][3];
    unsigned floor_count;
    double scale_points[2][3];
    unsigned scale_count;
    double scale;
    int flip_z;
} app_state_t;

static void app_state_init(app_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->threshold_a = DEFAULT_THRESHOLD;
    state->threshold_b = DEFAULT_THRESHOLD;
    state->min_area = DEFAULT_MIN_AREA;
    projection_identity3(state->floor_rot);
    state->scale = 1.0;
}

static int save_calibration(const calibration_t *calibration)
{
    FILE *out = fopen(CALIBRATION_FILE, "w");

    if (!out) {
        perror("fopen calibration");
        return -1;
    }

    fprintf(out, "STEREO_CALIBRATION_V1\n");
    fprintf(out, "P_A");
    for (int i = 0; i < 12; i++) {
        fprintf(out, " %.12g", calibration->p_a[i]);
    }
    fprintf(out, "\nP_B");
    for (int i = 0; i < 12; i++) {
        fprintf(out, " %.12g", calibration->p_b[i]);
    }
    fprintf(out, "\n");
    fclose(out);
    return 0;
}

static int load_calibration(calibration_t *calibration)
{
    FILE *in = fopen(CALIBRATION_FILE, "r");
    char tag[64];
    char name[16];

    memset(calibration, 0, sizeof(*calibration));
    if (!in) {
        return -1;
    }
    if (fscanf(in, "%63s", tag) != 1 ||
        strcmp(tag, "STEREO_CALIBRATION_V1") != 0) {
        fclose(in);
        return -1;
    }
    if (fscanf(in, "%15s", name) != 1 || strcmp(name, "P_A") != 0) {
        fclose(in);
        return -1;
    }
    for (int i = 0; i < 12; i++) {
        if (fscanf(in, "%lf", &calibration->p_a[i]) != 1) {
            fclose(in);
            return -1;
        }
    }
    if (fscanf(in, "%15s", name) != 1 || strcmp(name, "P_B") != 0) {
        fclose(in);
        return -1;
    }
    for (int i = 0; i < 12; i++) {
        if (fscanf(in, "%lf", &calibration->p_b[i]) != 1) {
            fclose(in);
            return -1;
        }
    }
    fclose(in);
    calibration->valid = 1;
    return 0;
}

static int read_valid_pair(const app_state_t *state, point2_t *a, point2_t *b)
{
    camera_moments_t moments_a;
    camera_moments_t moments_b;
    int valid = 0;

    if (camera_read_moment_pair(&moments_a, &moments_b, state->min_area,
                                &valid) < 0) {
        return -1;
    }
    if (!valid) {
        fprintf(stderr,
                "invalid beacon pair: areaA=%u areaB=%u min=%u\n",
                moments_a.area, moments_b.area, state->min_area);
        return 1;
    }

    a->x = moments_a.cx;
    a->y = moments_a.cy;
    b->x = moments_b.cx;
    b->y = moments_b.cy;
    return 0;
}

static int triangulate_raw(const calibration_t *calibration,
                           const point2_t *a, const point2_t *b,
                           double x[3])
{
    if (!calibration->valid) {
        return -1;
    }
    return projection_triangulate(calibration->p_a, calibration->p_b,
                                  a, b, x);
}

static int current_raw_point(const app_state_t *state, double raw[3])
{
    point2_t a;
    point2_t b;
    int ret = read_valid_pair(state, &a, &b);

    if (ret != 0) {
        return ret;
    }
    if (triangulate_raw(&state->calibration, &a, &b, raw) < 0) {
        fprintf(stderr, "triangulation failed\n");
        return -1;
    }
    return 0;
}

static int configure_defaults(app_state_t *state, camera_select_t cameras)
{
    camera_settings_t settings_a;
    camera_settings_t settings_b;
    int failed = 0;

    camera_settings_default_for(CAMERA_A, &settings_a);
    camera_settings_default_for(CAMERA_B, &settings_b);

    if ((cameras & CAMERA_A) &&
        camera_configure(CAMERA_A, &settings_a) < 0) {
        failed = 1;
    }
    if ((cameras & CAMERA_B) &&
        camera_configure(CAMERA_B, &settings_b) < 0) {
        failed = 1;
    }
    if (camera_set_thresholds(state->threshold_a, state->threshold_b) < 0) {
        failed = 1;
    }

    return failed ? -1 : 0;
}

static int solve_calibration(app_state_t *state, const point2_t *points_a,
                             const point2_t *points_b, size_t count)
{
    point2_t norm_a[MAX_CORRESPONDENCES];
    point2_t norm_b[MAX_CORRESPONDENCES];
    camera_pose_t pose;
    double f[9];
    double e[9];

    if (count < MIN_FUNDAMENTAL_POINTS) {
        fprintf(stderr, "need at least %u correspondences\n",
                MIN_FUNDAMENTAL_POINTS);
        return -1;
    }

    if (fundamental_estimate(points_a, points_b, count, f) < 0) {
        fprintf(stderr, "fundamental matrix estimation failed\n");
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        norm_a[i] = projection_normalize_pixel(K_A, points_a[i]);
        norm_b[i] = projection_normalize_pixel(K_B, points_b[i]);
    }

    essential_from_fundamental(K_A, K_B, f, e);
    if (essential_recover_pose(e, norm_a, norm_b, count, &pose) < 0) {
        fprintf(stderr, "essential pose recovery failed\n");
        return -1;
    }

    projection_make_left_camera(K_A, state->calibration.p_a);
    essential_projection_matrix(K_B, &pose, state->calibration.p_b);
    state->calibration.valid = 1;

    if (save_calibration(&state->calibration) < 0) {
        return -1;
    }

    printf("wrote projection matrices to %s\n", CALIBRATION_FILE);
    return 0;
}

static int read_key_timeout_ms(int timeout_ms)
{
    fd_set fds;
    struct timeval tv;
    unsigned char c;
    int ret;

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (ret <= 0) {
        return -1;
    }
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return -1;
    }
    return c;
}

static int set_raw_mode(const struct termios *old_term);

static int enable_raw(struct termios *old_term)
{
    if (tcgetattr(STDIN_FILENO, old_term) < 0) {
        perror("tcgetattr");
        return -1;
    }
    setvbuf(stdin, NULL, _IONBF, 0);
    return set_raw_mode(old_term);
}

static int set_raw_mode(const struct termios *old_term)
{
    struct termios raw = *old_term;

    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

static void restore_terminal(const struct termios *old_term)
{
    tcsetattr(STDIN_FILENO, TCSANOW, old_term);
}

static int prompt_double(const char *prompt, double *value)
{
    char line[LINE_LEN];
    char *end = NULL;

    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
        return -1;
    }
    *value = strtod(line, &end);
    while (end && isspace((unsigned char)*end)) {
        end++;
    }
    return end && *end == '\0' ? 0 : -1;
}

static int runtime_mode(app_state_t *state);

static int calibration_mode(app_state_t *state)
{
    point2_t points_a[MAX_CORRESPONDENCES];
    point2_t points_b[MAX_CORRESPONDENCES];
    size_t count = 0;
    struct termios old_term;

    printf("\nCalibration mode\n");
    printf("Q log valid beacon pair, C solve after %u pairs, G config defaults, X exit\n",
           MIN_FUNDAMENTAL_POINTS);

    if (enable_raw(&old_term) < 0) {
        return -1;
    }

    for (;;) {
        int key = read_key_timeout_ms(100);

        if (key < 0) {
            continue;
        }
        key = tolower(key);

        if (key == 'x') {
            restore_terminal(&old_term);
            printf("\nleaving calibration mode\n");
            return 0;
        }

        if (key == 'g') {
            restore_terminal(&old_term);
            configure_defaults(state, CAMERA_BOTH);
            if (enable_raw(&old_term) < 0) {
                return -1;
            }
            continue;
        }

        if (key == 'q') {
            point2_t a;
            point2_t b;
            int ret = read_valid_pair(state, &a, &b);

            if (ret == 0 && count < MAX_CORRESPONDENCES) {
                points_a[count] = a;
                points_b[count] = b;
                count++;
                printf("\n%zu: A=(%.2f, %.2f) B=(%.2f, %.2f)\n",
                       count, a.x, a.y, b.x, b.y);
                if (count >= MIN_FUNDAMENTAL_POINTS) {
                    printf("press C to solve projection matrices\n");
                }
            }
            continue;
        }

        if (key == 'c') {
            int ret;

            restore_terminal(&old_term);
            ret = solve_calibration(state, points_a, points_b, count);
            if (ret == 0) {
                runtime_mode(state);
                return 0;
            }
            if (enable_raw(&old_term) < 0) {
                return -1;
            }
        }
    }
}

static void compute_floor_rotation(app_state_t *state)
{
    if (projection_floor_rotation_from_points(state->floor_points[0],
                                              state->floor_points[1],
                                              state->floor_points[2],
                                              state->floor_rot) < 0) {
        fprintf(stderr, "floor points are degenerate\n");
        return;
    }
    state->floor_set = 1;
    printf("\nfloor normal aligned to +Z\n");
}

static void log_floor_point(app_state_t *state, const double raw[3])
{
    double shifted[3];

    if (state->floor_count >= 3) {
        printf("\nfloor already calibrated; restarting floor collection\n");
        state->floor_count = 0;
        state->floor_set = 0;
        projection_identity3(state->floor_rot);
    }

    shifted[0] = raw[0] - (state->origin_set ? state->origin[0] : 0.0);
    shifted[1] = raw[1] - (state->origin_set ? state->origin[1] : 0.0);
    shifted[2] = raw[2] - (state->origin_set ? state->origin[2] : 0.0);
    memcpy(state->floor_points[state->floor_count], shifted, sizeof(shifted));
    state->floor_count++;
    printf("\nfloor point %u/3 logged\n", state->floor_count);

    if (state->floor_count == 3) {
        compute_floor_rotation(state);
    }
}

static int log_scale_point(app_state_t *state, const double raw[3],
                           const struct termios *old_term)
{
    double p[3];

    projection_apply_transform_unscaled(raw, state->origin, state->origin_set,
                                        state->floor_rot, p);
    if (state->scale_count >= 2) {
        printf("\nscale already calibrated; restarting scale collection\n");
        state->scale_count = 0;
        state->scale = 1.0;
    }

    memcpy(state->scale_points[state->scale_count], p, sizeof(p));
    state->scale_count++;
    printf("\nscale point %u/2 logged\n", state->scale_count);

    if (state->scale_count == 2) {
        double dist;
        double measured;

        dist = projection_distance3(state->scale_points[0],
                                    state->scale_points[1]);
        if (dist < 1.0e-12) {
            fprintf(stderr, "scale points are identical\n");
            state->scale_count = 0;
            return 0;
        }

        restore_terminal(old_term);
        if (prompt_double("real distance between scale points: ", &measured) < 0 ||
            measured <= 0.0) {
            fprintf(stderr, "invalid scale distance\n");
            if (set_raw_mode(old_term) < 0) {
                return -1;
            }
            return 0;
        }
        state->scale = measured / dist;
        printf("scale set to %.6f\n", state->scale);
        if (set_raw_mode(old_term) < 0) {
            return -1;
        }
    }

    return 0;
}

static int runtime_mode(app_state_t *state)
{
    struct termios old_term;
    double raw[3] = {0.0, 0.0, 0.0};
    double xyz[3] = {0.0, 0.0, 0.0};
    int have_point = 0;

    if (!state->calibration.valid) {
        fprintf(stderr, "projection calibration is not valid\n");
        return -1;
    }

    printf("\nRuntime triangulation mode\n");
    printf("O set origin, F log floor point, S log scale point, Z flip z, X exit\n");

    if (enable_raw(&old_term) < 0) {
        return -1;
    }

    for (;;) {
        int valid_read = current_raw_point(state, raw);
        int key;

        if (valid_read == 0) {
            projection_apply_transform(raw, state->origin, state->origin_set,
                                       state->floor_rot, state->scale, xyz);
            if (state->flip_z) {
                xyz[2] = -xyz[2];
            }
            have_point = 1;
            printf("\rxyz = %.4f, %.4f, %.4f        ",
                   xyz[0], xyz[1], xyz[2]);
            fflush(stdout);
        }

        key = read_key_timeout_ms(1);
        if (key < 0) {
            continue;
        }
        key = tolower(key);

        if (key == 'x') {
            restore_terminal(&old_term);
            printf("\nleaving runtime mode\n");
            return 0;
        }

        if (key == 'z') {
            state->flip_z = !state->flip_z;
            printf("\nflip z %s\n", state->flip_z ? "on" : "off");
            continue;
        }

        if (!have_point) {
            printf("\nno valid point available for command\n");
            continue;
        }

        if (key == 'o') {
            memcpy(state->origin, raw, sizeof(state->origin));
            state->origin_set = 1;
            printf("\norigin set\n");
        } else if (key == 'f') {
            log_floor_point(state, raw);
        } else if (key == 's') {
            if (log_scale_point(state, raw, &old_term) < 0) {
                restore_terminal(&old_term);
                return -1;
            }
        }
    }
}

static int split_line(char *line, char **argv)
{
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");

    while (tok && argc < MAX_ARGS) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    return argc;
}

static int parse_config_key(app_state_t *state, const char *arg,
                            camera_settings_t *settings_a,
                            camera_settings_t *settings_b,
                            camera_select_t cameras)
{
    const char *value = strchr(arg, '=');

    if (!value) {
        if ((cameras & CAMERA_A) &&
            camera_config_parse_setting(arg, settings_a) < 0) {
            return -1;
        }
        if ((cameras & CAMERA_B) &&
            camera_config_parse_setting(arg, settings_b) < 0) {
            return -1;
        }
        return 0;
    }

    if (strncmp(arg, "threshold=", 10) == 0 ||
        strncmp(arg, "th=", 3) == 0) {
        unsigned th;
        const char *v = value + 1;
        if (parse_unsigned_arg(v, CAMERA_REG8_MAX, &th) < 0) {
            return -1;
        }
        state->threshold_a = th;
        state->threshold_b = th;
        return 0;
    }
    if (strncmp(arg, "thresholdA=", 11) == 0 ||
        strncmp(arg, "thA=", 4) == 0) {
        if (parse_unsigned_arg(value + 1, CAMERA_REG8_MAX,
                               &state->threshold_a) < 0) {
            return -1;
        }
        return 0;
    }
    if (strncmp(arg, "thresholdB=", 11) == 0 ||
        strncmp(arg, "thB=", 4) == 0) {
        if (parse_unsigned_arg(value + 1, CAMERA_REG8_MAX,
                               &state->threshold_b) < 0) {
            return -1;
        }
        return 0;
    }
    if (strncmp(arg, "minarea=", 8) == 0 ||
        strncmp(arg, "min_area=", 9) == 0) {
        if (parse_unsigned_arg(value + 1, FRAME_PIXELS, &state->min_area) < 0) {
            return -1;
        }
        return 0;
    }

    if ((cameras & CAMERA_A) &&
        camera_config_parse_setting(arg, settings_a) < 0) {
        return -1;
    }
    if ((cameras & CAMERA_B) &&
        camera_config_parse_setting(arg, settings_b) < 0) {
        return -1;
    }
    return 0;
}

static int command_config(app_state_t *state, int argc, char **argv)
{
    camera_select_t cameras = CAMERA_BOTH;
    camera_settings_t settings_a;
    camera_settings_t settings_b;
    int argi = 0;
    int failed = 0;

    camera_settings_default_for(CAMERA_A, &settings_a);
    camera_settings_default_for(CAMERA_B, &settings_b);

    if (argc > 0 && camera_parse_select(argv[0], &cameras) == 0) {
        argi = 1;
    }

    while (argi < argc) {
        if (parse_config_key(state, argv[argi], &settings_a, &settings_b,
                             cameras) < 0) {
            fprintf(stderr, "invalid config option: %s\n", argv[argi]);
            return -1;
        }
        argi++;
    }

    if ((cameras & CAMERA_A) && camera_configure(CAMERA_A, &settings_a) < 0) {
        failed = 1;
    }
    if ((cameras & CAMERA_B) && camera_configure(CAMERA_B, &settings_b) < 0) {
        failed = 1;
    }
    if (camera_set_thresholds(state->threshold_a, state->threshold_b) < 0) {
        failed = 1;
    }

    printf("min valid area=%u\n", state->min_area);
    return failed ? -1 : 0;
}

static int command_capture(int argc, char **argv)
{
    camera_select_t camera = CAMERA_A;
    int argi = 0;

    if (argi < argc && camera_parse_single(argv[argi], &camera) == 0) {
        argi++;
    }
    if (argi != argc) {
        fprintf(stderr, "usage: capture [A|B]\n");
        return -1;
    }
    return camera_capture_serial(camera, stdout);
}

static int command_debug(void)
{
    struct termios old_term;
    int ret;

    printf("Debug stream; press any key to stop.\n");
    if (enable_raw(&old_term) < 0) {
        return -1;
    }
    ret = camera_debug_stream(30);
    restore_terminal(&old_term);
    return ret;
}

static void print_help(void)
{
    printf("commands:\n");
    printf("  config [A|B|both] [gain=...] [agc=...] [aec=...] [exposure=...]\n");
    printf("         [threshold=...] [thresholdA=...] [thresholdB=...] [minarea=...]\n");
    printf("  capture [A|B]       emit one serial frame block\n");
    printf("  debug              stream area/u/v/centroid output until keypress\n");
    printf("  calibrate          enter correspondence logging mode\n");
    printf("  runtime            enter triangulation mode (Z toggles z sign)\n");
    printf("  status             show hardware status\n");
    printf("  quit\n");
}

static int prompt_initial_mode(app_state_t *state)
{
    char line[LINE_LEN];

    if (!state->calibration.valid) {
        printf("No valid %s; calibration is required.\n", CALIBRATION_FILE);
        return calibration_mode(state);
    }

    printf("Loaded %s.\n", CALIBRATION_FILE);
    printf("Enter runtime, recalibrate, or command menu? [r/c/x]: ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
        return -1;
    }
    if (tolower((unsigned char)line[0]) == 'c') {
        return calibration_mode(state);
    }
    if (tolower((unsigned char)line[0]) == 'r') {
        return runtime_mode(state);
    }
    return 0;
}

int main(void)
{
    app_state_t state;
    char line[LINE_LEN];

    app_state_init(&state);
    if (load_calibration(&state.calibration) == 0) {
        state.calibration.valid = 1;
    }

    printf("stereo terminal\n");
    printf("intrinsics loaded for camera A and B\n");
    printf("applying startup camera config\n");
    if (configure_defaults(&state, CAMERA_BOTH) < 0) {
        printf("startup config reported an error; continuing anyway\n");
    }
    if (prompt_initial_mode(&state) < 0) {
        printf("initial mode ended with an error; entering command prompt\n");
    }

    print_help();
    while (1) {
        char *argv[MAX_ARGS];
        int argc;

        printf("stereo> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        argc = split_line(line, argv);
        if (argc == 0) {
            continue;
        }

        if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "exit") == 0) {
            break;
        } else if (strcmp(argv[0], "help") == 0) {
            print_help();
        } else if (strcmp(argv[0], "config") == 0 ||
                   strcmp(argv[0], "configure") == 0) {
            command_config(&state, argc - 1, argv + 1);
        } else if (strcmp(argv[0], "capture") == 0) {
            command_capture(argc - 1, argv + 1);
        } else if (strcmp(argv[0], "debug") == 0) {
            command_debug();
        } else if (strcmp(argv[0], "calibrate") == 0) {
            calibration_mode(&state);
        } else if (strcmp(argv[0], "runtime") == 0) {
            runtime_mode(&state);
        } else if (strcmp(argv[0], "status") == 0) {
            camera_print_status();
        } else {
            fprintf(stderr, "unknown command: %s\n", argv[0]);
            print_help();
        }
    }

    return 0;
}
