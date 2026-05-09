#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define IMGPROC_BASE 0xFF200000u
#define IMGPROC_SPAN 0x1000u

#define REG32(byte_offset) ((byte_offset) / 4)

#define IMG_AREA_A REG32(0x00u)
#define IMG_U_A REG32(0x04u)
#define IMG_V_A REG32(0x08u)
#define IMG_AREA_B REG32(0x0cu)
#define IMG_U_B REG32(0x10u)
#define IMG_V_B REG32(0x14u)
#define IMG_DONE REG32(0x18u)
#define IMG_CONTROL REG32(0x1cu)

#define DONE_A (1u << 0)
#define DONE_B (1u << 1)
#define DONE_MOMENTS (DONE_A | DONE_B)

#define STORE_MASK 0x3u
#define STORE_NONE 0u

#define THRESH_A_SHIFT 8
#define THRESH_B_SHIFT 16
#define THRESH_MASK 0xffu

#define CLEAR_TIMEOUT_POLLS 10000000u
#define DONE_TIMEOUT_POLLS 100000000u

#define WIDTH 640.0
#define HEIGHT 480.0
#define DEFAULT_MATCH_TARGET 30u
#define MIN_MATCH_TARGET 9u
#define PRINT_EVERY 30u
#define NS_PER_SEC 1000000000.0

#define CORDIC_ITERATIONS 16
#define CORDIC_SCALE 10000
#define PI_DEG100 18000
#define HALF_PI_DEG100 9000

static const int32_t cordic_atan_deg100[CORDIC_ITERATIONS] = {
    4500, 2657, 1404, 713, 358, 179, 90, 45,
    22, 11, 6, 3, 1, 1, 0, 0
};

static void clear_done(volatile uint32_t *regs, uint32_t bits)
{
    regs[IMG_DONE] = ~bits;
}

static int wait_clear(volatile uint32_t *regs, uint32_t bits, const char *name)
{
    for (unsigned polls = 0; (regs[IMG_DONE] & bits) != 0; polls++) {
        if (polls >= CLEAR_TIMEOUT_POLLS) {
            fprintf(stderr, "%s: timeout clearing DONE; DONE=0x%08x\n",
                    name, regs[IMG_DONE]);
            return -1;
        }
    }

    return 0;
}

static int wait_done(volatile uint32_t *regs, uint32_t bits, const char *name)
{
    for (unsigned polls = 0; (regs[IMG_DONE] & bits) != bits; polls++) {
        if (polls >= DONE_TIMEOUT_POLLS) {
            fprintf(stderr, "%s: timeout waiting for DONE; DONE=0x%08x\n",
                    name, regs[IMG_DONE]);
            return -1;
        }
    }

    return 0;
}

static int parse_unsigned(const char *text, unsigned min_value,
                          unsigned max_value, unsigned *value)
{
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno || *end || parsed < min_value || parsed > max_value) {
        return -1;
    }

    *value = (unsigned)parsed;
    return 0;
}

static double elapsed_seconds(const struct timespec *start,
                              const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / NS_PER_SEC;
}

static void usage(const char *name)
{
    fprintf(stderr,
            "usage: %s [threshold_a [threshold_b [focal_x [focal_y [match_target]]]]]\n",
            name);
    fprintf(stderr, "  threshold values: 0..255\n");
    fprintf(stderr, "  focal values are rough pixel focal lengths used only for CORDIC angle output\n");
    fprintf(stderr, "  match_target minimum is %u, default is %u\n",
            MIN_MATCH_TARGET, DEFAULT_MATCH_TARGET);
}

static int32_t cordic_atan2_deg100(int32_t y, int32_t x)
{
    int32_t angle = 0;
    int32_t xi;
    int32_t yi;

    if (x == 0 && y == 0) {
        return 0;
    }

    if (x < 0) {
        x = -x;
        y = -y;
        angle = (y >= 0) ? PI_DEG100 : -PI_DEG100;
    }

    xi = x;
    yi = y;

    for (int i = 0; i < CORDIC_ITERATIONS; i++) {
        int32_t x_shift = xi >> i;
        int32_t y_shift = yi >> i;

        if (yi > 0) {
            xi += y_shift;
            yi -= x_shift;
            angle += cordic_atan_deg100[i];
        } else {
            xi -= y_shift;
            yi += x_shift;
            angle -= cordic_atan_deg100[i];
        }
    }

    if (angle > PI_DEG100) {
        angle -= 2 * PI_DEG100;
    } else if (angle < -PI_DEG100) {
        angle += 2 * PI_DEG100;
    }

    return angle;
}

static void print_angle(int32_t deg100)
{
    printf("%ld.%02ld",
           (long)(deg100 / 100),
           labs((long)(deg100 % 100)));
}

static int centroid_from_moments(uint32_t area, uint32_t sx, uint32_t sy,
                                 double *x, double *y)
{
    if (area == 0) {
        return 0;
    }

    *x = (double)sx / (double)area;
    *y = (double)sy / (double)area;
    return 1;
}

int main(int argc, char **argv)
{
    unsigned threshold_a = 180;
    unsigned threshold_b = 180;
    unsigned focal_x = 500;
    unsigned focal_y = 500;
    unsigned match_target = DEFAULT_MATCH_TARGET;

    if (argc > 6) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 2 &&
        parse_unsigned(argv[1], 0, THRESH_MASK, &threshold_a) < 0) {
        usage(argv[0]);
        return 2;
    }

    threshold_b = threshold_a;

    if (argc >= 3 &&
        parse_unsigned(argv[2], 0, THRESH_MASK, &threshold_b) < 0) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 4 && parse_unsigned(argv[3], 1, 10000, &focal_x) < 0) {
        usage(argv[0]);
        return 2;
    }

    focal_y = focal_x;

    if (argc >= 5 && parse_unsigned(argv[4], 1, 10000, &focal_y) < 0) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 6 &&
        parse_unsigned(argv[5], MIN_MATCH_TARGET, 10000, &match_target) < 0) {
        usage(argv[0]);
        return 2;
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    void *map = mmap(NULL, IMGPROC_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd, IMGPROC_BASE);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    volatile uint32_t *regs = (volatile uint32_t *)map;

    regs[IMG_CONTROL] =
        (regs[IMG_CONTROL] &
         ~(STORE_MASK | (THRESH_MASK << THRESH_A_SHIFT) |
           (THRESH_MASK << THRESH_B_SHIFT))) |
        STORE_NONE |
        (threshold_a << THRESH_A_SHIFT) |
        (threshold_b << THRESH_B_SHIFT);

    printf("thresholdA=%u thresholdB=%u focalX=%u focalY=%u matchTarget=%u CONTROL=0x%08x\n",
           threshold_a, threshold_b, focal_x, focal_y, match_target,
           regs[IMG_CONTROL]);

    FILE *matches = fopen("stereo_matches.csv", "w");
    if (!matches) {
        perror("fopen stereo_matches.csv");
        munmap(map, IMGPROC_SPAN);
        close(fd);
        return 1;
    }

    fprintf(matches,
            "sample,xA,yA,xB,yB,angleXA_deg100,angleYA_deg100,angleXB_deg100,angleYB_deg100,rawDisparity\n");

    clear_done(regs, DONE_MOMENTS);

    if (wait_clear(regs, DONE_MOMENTS, "initial moments") < 0 ||
        wait_done(regs, DONE_MOMENTS, "initial moments") < 0) {
        fclose(matches);
        munmap(map, IMGPROC_SPAN);
        close(fd);
        return 1;
    }

    clear_done(regs, DONE_MOMENTS);

    if (wait_clear(regs, DONE_MOMENTS, "initial moments discard") < 0) {
        fclose(matches);
        munmap(map, IMGPROC_SPAN);
        close(fd);
        return 1;
    }

    printf("areaA,uA,vA,xA,yA,angleXA,angleYA,areaB,uB,vB,xB,yB,angleXB,angleYB,matchCount,matchTarget,rawDisparity,done,hz\n");

    struct timespec last_print;
    int have_last_print = 0;
    unsigned sample = 0;
    unsigned match_count = 0;

    while (match_count < match_target) {
        if (wait_done(regs, DONE_MOMENTS, "moments") < 0) {
            break;
        }

        uint32_t done_snapshot = regs[IMG_DONE];

        uint32_t area_a = regs[IMG_AREA_A];
        uint32_t u_a = regs[IMG_U_A];
        uint32_t v_a = regs[IMG_V_A];

        uint32_t area_b = regs[IMG_AREA_B];
        uint32_t u_b = regs[IMG_U_B];
        uint32_t v_b = regs[IMG_V_B];

        clear_done(regs, DONE_MOMENTS);

        if (wait_clear(regs, DONE_MOMENTS, "moments") < 0) {
            break;
        }

        sample++;

        double x_a = 0.0;
        double y_a = 0.0;
        double x_b = 0.0;
        double y_b = 0.0;

        int valid_a = centroid_from_moments(area_a, u_a, v_a, &x_a, &y_a);
        int valid_b = centroid_from_moments(area_b, u_b, v_b, &x_b, &y_b);

        int32_t angle_x_a = 0;
        int32_t angle_y_a = 0;
        int32_t angle_x_b = 0;
        int32_t angle_y_b = 0;
        int32_t raw_disparity = 0;

        if (valid_a) {
            int32_t dx_a = (int32_t)((x_a - WIDTH / 2.0) * CORDIC_SCALE);
            int32_t dy_a = (int32_t)((y_a - HEIGHT / 2.0) * CORDIC_SCALE);
            angle_x_a = cordic_atan2_deg100(dx_a, (int32_t)focal_x * CORDIC_SCALE);
            angle_y_a = cordic_atan2_deg100(dy_a, (int32_t)focal_y * CORDIC_SCALE);
        }

        if (valid_b) {
            int32_t dx_b = (int32_t)((x_b - WIDTH / 2.0) * CORDIC_SCALE);
            int32_t dy_b = (int32_t)((y_b - HEIGHT / 2.0) * CORDIC_SCALE);
            angle_x_b = cordic_atan2_deg100(dx_b, (int32_t)focal_x * CORDIC_SCALE);
            angle_y_b = cordic_atan2_deg100(dy_b, (int32_t)focal_y * CORDIC_SCALE);
        }

        if (valid_a && valid_b) {
            raw_disparity = (int32_t)((x_a - x_b) * 1000.0);

            match_count++;
            fprintf(matches, "%u,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%d,%d\n",
                    match_count, x_a, y_a, x_b, y_b,
                    angle_x_a, angle_y_a, angle_x_b, angle_y_b,
                    raw_disparity);
            fflush(matches);
        }

        if (sample % PRINT_EVERY == 0 || match_count == match_target) {
            struct timespec now;
            double hz = 0.0;

            clock_gettime(CLOCK_MONOTONIC, &now);
            if (have_last_print) {
                double dt = elapsed_seconds(&last_print, &now);
                if (dt > 0.0) {
                    hz = (double)PRINT_EVERY / dt;
                }
            }
            last_print = now;
            have_last_print = 1;

            printf("%u,%u,%u,%.2f,%.2f,",
                   area_a, u_a, v_a, valid_a ? x_a : -1.0, valid_a ? y_a : -1.0);
            print_angle(angle_x_a);
            printf(",");
            print_angle(angle_y_a);
            printf(",%u,%u,%u,%.2f,%.2f,",
                   area_b, u_b, v_b, valid_b ? x_b : -1.0, valid_b ? y_b : -1.0);
            print_angle(angle_x_b);
            printf(",");
            print_angle(angle_y_b);
            printf(",%u,%u,%d,0x%08x,%.2f\n",
                   match_count, match_target, raw_disparity, done_snapshot, hz);
            fflush(stdout);
        }
    }

    fclose(matches);

    printf("collected %u stereo matches in stereo_matches.csv\n", match_count);

    munmap(map, IMGPROC_SPAN);
    close(fd);

    return match_count >= MIN_MATCH_TARGET ? 0 : 1;
}

