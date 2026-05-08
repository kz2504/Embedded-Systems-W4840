#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define IMGPROC_BASE 0xFF200000u
#define IMGPROC_SPAN 0x1000u

#define REG32(byte_offset) ((byte_offset) / 4)

#define IMG_AREA_A   REG32(0x00u)
#define IMG_U_A      REG32(0x04u)
#define IMG_V_A      REG32(0x08u)
#define IMG_AREA_B   REG32(0x0cu)
#define IMG_U_B      REG32(0x10u)
#define IMG_V_B      REG32(0x14u)
#define IMG_DONE     REG32(0x18u)
#define IMG_CONTROL  REG32(0x1cu)

#define DONE_A (1u << 0)
#define DONE_B (1u << 1)
#define DONE_MOMENTS (DONE_A | DONE_B)

#define STORE_MASK 0x3u
#define STORE_NONE 0u

#define THRESH_A_SHIFT 8
#define THRESH_B_SHIFT 16
#define THRESH_MASK 0xffu

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define CENTER_X (FRAME_WIDTH / 2)
#define CENTER_Y (FRAME_HEIGHT / 2)

#define DEFAULT_FOCAL_X 500
#define DEFAULT_FOCAL_Y 500

#define DEFAULT_MATCH_TARGET 30
#define MIN_MATCH_TARGET 9

#define CLEAR_TIMEOUT_POLLS 10000000u
#define DONE_TIMEOUT_POLLS 100000000u
#define NS_PER_SEC 1000000000.0
#define PRINT_EVERY 30

#define CORDIC_ITER 16

static const int32_t atan_table[CORDIC_ITER] = {
    4500, 2657, 1404, 713,
    358, 179, 90, 45,
    22, 11, 6, 3,
    1, 1, 0, 0
};

static int32_t cordic_atan2_deg100(int32_t y, int32_t x)
{
    int32_t angle = 0;

    for (int i = 0; i < CORDIC_ITER; i++) {
        int32_t x_new;
        int32_t y_new;

        if (y > 0) {
            x_new = x + (y >> i);
            y_new = y - (x >> i);
            angle += atan_table[i];
        } else {
            x_new = x - (y >> i);
            y_new = y + (x >> i);
            angle -= atan_table[i];
        }

        x = x_new;
        y = y_new;
    }

    return angle;
}

static void print_deg100(int32_t angle)
{
    int32_t whole = angle / 100;
    int32_t frac = angle % 100;

    if (frac < 0)
        frac = -frac;

    printf("%" PRId32 ".%02" PRId32, whole, frac);
}

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

static int parse_u32_limit(const char *text, unsigned max_value, unsigned *out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);

    if (errno || *end || value > max_value)
        return -1;

    *out = (unsigned)value;
    return 0;
}

static void usage(const char *name)
{
    fprintf(stderr,
            "usage: %s [threshold_a [threshold_b [focal_x focal_y match_target]]]\n",
            name);
    fprintf(stderr, "  thresholds: 0..255\n");
    fprintf(stderr, "  focal_x/focal_y: focal lengths in pixels, temporary until OpenCV K is used\n");
    fprintf(stderr, "  match_target: minimum 9, default 30\n");
}

static double elapsed_seconds(const struct timespec *start,
                              const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / NS_PER_SEC;
}

int main(int argc, char **argv)
{
    unsigned threshold_a = 0;
    unsigned threshold_b = 0;
    unsigned focal_x = DEFAULT_FOCAL_X;
    unsigned focal_y = DEFAULT_FOCAL_Y;
    unsigned match_target = DEFAULT_MATCH_TARGET;

    if (argc > 6) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 2 && parse_u32_limit(argv[1], THRESH_MASK, &threshold_a) < 0) {
        usage(argv[0]);
        return 2;
    }

    threshold_b = threshold_a;

    if (argc >= 3 && parse_u32_limit(argv[2], THRESH_MASK, &threshold_b) < 0) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 4 && parse_u32_limit(argv[3], 100000u, &focal_x) < 0) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 5 && parse_u32_limit(argv[4], 100000u, &focal_y) < 0) {
        usage(argv[0]);
        return 2;
    }

    if (argc >= 6 && parse_u32_limit(argv[5], 100000u, &match_target) < 0) {
        usage(argv[0]);
        return 2;
    }

    if (focal_x == 0 || focal_y == 0) {
        fprintf(stderr, "focal_x and focal_y must be nonzero\n");
        return 2;
    }

    if (match_target < MIN_MATCH_TARGET) {
        fprintf(stderr, "match_target must be at least %u\n", MIN_MATCH_TARGET);
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

    FILE *matches = fopen("stereo_matches.csv", "w");
    if (!matches) {
        perror("fopen stereo_matches.csv");
        munmap(map, IMGPROC_SPAN);
        close(fd);
        return 1;
    }

    fprintf(matches,
            "sample,xA,yA,xB,yB,angleXA_deg100,angleYA_deg100,angleXB_deg100,angleYB_deg100,rawDisparity\n");

    volatile uint32_t *regs = (volatile uint32_t *)map;
    struct timespec last_print;
    int have_last_print = 0;
    unsigned sample = 0;
    unsigned collected_matches = 0;

    regs[IMG_CONTROL] =
        (regs[IMG_CONTROL] &
         ~(STORE_MASK | (THRESH_MASK << THRESH_A_SHIFT) |
           (THRESH_MASK << THRESH_B_SHIFT))) |
        STORE_NONE |
        (threshold_a << THRESH_A_SHIFT) |
        (threshold_b << THRESH_B_SHIFT);

    printf("frame store off, thresholdA=%u thresholdB=%u focalX=%u focalY=%u matchTarget=%u CONTROL=0x%08x\n",
           threshold_a, threshold_b, focal_x, focal_y, match_target,
           regs[IMG_CONTROL]);

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

    while (1) {
        uint32_t area_a;
        uint32_t u_a;
        uint32_t v_a;
        uint32_t area_b;
        uint32_t u_b;
        uint32_t v_b;
        uint32_t done_snapshot;

        if (wait_done(regs, DONE_MOMENTS, "moments") < 0)
            break;

        done_snapshot = regs[IMG_DONE];

        area_a = regs[IMG_AREA_A];
        u_a = regs[IMG_U_A];
        v_a = regs[IMG_V_A];

        area_b = regs[IMG_AREA_B];
        u_b = regs[IMG_U_B];
        v_b = regs[IMG_V_B];

        clear_done(regs, DONE_MOMENTS);

        if (wait_clear(regs, DONE_MOMENTS, "moments") < 0)
            break;

        sample++;

        uint32_t x_a = 0;
        uint32_t y_a = 0;
        uint32_t x_b = 0;
        uint32_t y_b = 0;

        int valid_a = 0;
        int valid_b = 0;

        int32_t angle_x_a = 0;
        int32_t angle_y_a = 0;
        int32_t angle_x_b = 0;
        int32_t angle_y_b = 0;
        int32_t raw_disparity = 0;

        if (area_a != 0) {
            valid_a = 1;
            x_a = u_a / area_a;
            y_a = v_a / area_a;

            angle_x_a = cordic_atan2_deg100((int32_t)x_a - CENTER_X,
                                             (int32_t)focal_x);
            angle_y_a = cordic_atan2_deg100(CENTER_Y - (int32_t)y_a,
                                             (int32_t)focal_y);
        }

        if (area_b != 0) {
            valid_b = 1;
            x_b = u_b / area_b;
            y_b = v_b / area_b;

            angle_x_b = cordic_atan2_deg100((int32_t)x_b - CENTER_X,
                                             (int32_t)focal_x);
            angle_y_b = cordic_atan2_deg100(CENTER_Y - (int32_t)y_b,
                                             (int32_t)focal_y);
        }

        if (valid_a && valid_b) {
            raw_disparity = (int32_t)x_a - (int32_t)x_b;

            if (collected_matches < match_target) {
                collected_matches++;

                fprintf(matches,
                        "%u,%u,%u,%u,%u,%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 "\n",
                        collected_matches,
                        x_a, y_a, x_b, y_b,
                        angle_x_a, angle_y_a,
                        angle_x_b, angle_y_b,
                        raw_disparity);
                fflush(matches);

                if (collected_matches == match_target) {
                    printf("collected %u stereo matches in stereo_matches.csv\n",
                           collected_matches);
                }
            }
        }

        if (sample % PRINT_EVERY == 0) {
            struct timespec now;
            double hz = 0.0;

            clock_gettime(CLOCK_MONOTONIC, &now);

            if (have_last_print) {
                double dt = elapsed_seconds(&last_print, &now);
                if (dt > 0.0)
                    hz = (double)PRINT_EVERY / dt;
            }

            last_print = now;
            have_last_print = 1;

            printf("%u,%u,%u,%u,%u,", area_a, u_a, v_a, x_a, y_a);
            print_deg100(angle_x_a);
            printf(",");
            print_deg100(angle_y_a);

            printf(",%u,%u,%u,%u,%u,", area_b, u_b, v_b, x_b, y_b);
            print_deg100(angle_x_b);
            printf(",");
            print_deg100(angle_y_b);

            printf(",%u,%u,%" PRId32 ",0x%08x,%.2f\n",
                   collected_matches, match_target, raw_disparity,
                   done_snapshot, hz);

            fflush(stdout);
        }
    }

    fclose(matches);
    munmap(map, IMGPROC_SPAN);
    close(fd);
    return 1;
}

