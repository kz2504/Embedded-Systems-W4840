/*
 * ir_beacon_cordic.c
 *
 * Software IR beacon tracker using centroid + CORDIC angle calculation.
 *
 * Reads frames from FPGA imgproc registers:
 *   0 CONTROL: bit 0 = DONE, write 0 to clear/start next capture
 *   1 INDEX:   frame word index
 *   2 DATA:    four grayscale pixels packed as {p3, p2, p1, p0}
 *
 * Compile:
 *   gcc ir_beacon_cordic.c -o ir_beacon_cordic -O2
 *
 * Run:
 *   sudo ./ir_beacon_cordic --base 0xFF200000 --threshold 180
 *
 * Base address should be:
 *   0xFF200000 + imgproc_base_offset_from_qsys
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define REG_CONTROL 0
#define REG_INDEX   1
#define REG_DATA    2

#define FRAME_WIDTH  640
#define FRAME_HEIGHT 480
#define FRAME_WORDS  ((FRAME_WIDTH * FRAME_HEIGHT) / 4)

#define CENTER_X (FRAME_WIDTH / 2)
#define CENTER_Y (FRAME_HEIGHT / 2)

#define MAP_SIZE 0x1000
#define TIMEOUT_MS 2000

#define CORDIC_ITER 16

static volatile uint32_t *regs;

/*
 * atan(2^-i) in degrees * 100.
 * Example: 45 degrees = 4500.
 */
static const int32_t atan_table[CORDIC_ITER] = {
    4500, 2657, 1404, 713,
    358, 179, 90, 45,
    22, 11, 6, 3,
    1, 1, 0, 0
};

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static uint32_t read_reg(unsigned reg)
{
    return regs[reg];
}

static void write_reg(unsigned reg, uint32_t value)
{
    regs[reg] = value;
}

/*
 * CORDIC atan2 approximation.
 * Returns angle in degrees * 100.
 */
static int32_t cordic_atan2_deg100(int32_t y, int32_t x)
{
    int32_t angle = 0;
    int32_t x_new;
    int32_t y_new;
    int i;

    if (x < 0) {
        if (y >= 0)
            angle = 18000;
        else
            angle = -18000;

        x = -x;
        y = -y;
    }

    for (i = 0; i < CORDIC_ITER; i++) {
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

    if (angle > 18000)
        angle -= 36000;
    if (angle < -18000)
        angle += 36000;

    return angle;
}

static int wait_done(void)
{
    uint64_t start = now_ms();

    while ((read_reg(REG_CONTROL) & 1u) == 0u) {
        if (now_ms() - start > TIMEOUT_MS) {
            fprintf(stderr, "Timed out waiting for capture DONE\n");
            return 0;
        }
        usleep(1000);
    }

    return 1;
}

static int capture_frame(uint8_t *frame)
{
    int i;

    write_reg(REG_CONTROL, 0);

    if (!wait_done())
        return 0;

    for (i = 0; i < FRAME_WORDS; i++) {
        uint32_t word;

        write_reg(REG_INDEX, (uint32_t)i);
        word = read_reg(REG_DATA);

        frame[4 * i + 0] = (uint8_t)(word & 0xffu);
        frame[4 * i + 1] = (uint8_t)((word >> 8) & 0xffu);
        frame[4 * i + 2] = (uint8_t)((word >> 16) & 0xffu);
        frame[4 * i + 3] = (uint8_t)((word >> 24) & 0xffu);
    }

    return 1;
}

static int calibrate_threshold(uint8_t *frame, int cal_frames, int margin)
{
    int sum_max = 0;
    int f;

    printf("Calibration: keep the IR beacon out of view.\n");

    for (f = 0; f < cal_frames; f++) {
        int max_pixel = 0;
        int i;

        if (!capture_frame(frame))
            return -1;

        for (i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
            if (frame[i] > max_pixel)
                max_pixel = frame[i];
        }

        sum_max += max_pixel;
        printf("  calibration frame %d/%d max=%d\n", f + 1, cal_frames, max_pixel);
    }

    int threshold = (sum_max / cal_frames) + margin;

    if (threshold > 255)
        threshold = 255;
    if (threshold < 0)
        threshold = 0;

    printf("Calibrated threshold = %d\n", threshold);
    return threshold;
}

static int find_beacon(uint8_t *frame, int threshold, int min_pixels,
                       int *out_x, int *out_y, uint64_t *out_count)
{
    uint64_t sum_x = 0;
    uint64_t sum_y = 0;
    uint64_t count = 0;
    int x;
    int y;

    for (y = 0; y < FRAME_HEIGHT; y++) {
        for (x = 0; x < FRAME_WIDTH; x++) {
            uint8_t pixel = frame[y * FRAME_WIDTH + x];

            if (pixel >= threshold) {
                sum_x += (uint64_t)x;
                sum_y += (uint64_t)y;
                count++;
            }
        }
    }

    *out_count = count;

    if (count < (uint64_t)min_pixels)
        return 0;

    *out_x = (int)(sum_x / count);
    *out_y = (int)(sum_y / count);
    return 1;
}

static void print_angle(const char *label, int32_t angle_deg100)
{
    int32_t whole = angle_deg100 / 100;
    int32_t frac = angle_deg100 % 100;

    if (frac < 0)
        frac = -frac;

    printf("%s=%" PRId32 ".%02" PRId32 " deg", label, whole, frac);
}

static void save_pgm(const char *filename, uint8_t *frame)
{
    FILE *f = fopen(filename, "wb");

    if (!f) {
        fprintf(stderr, "Could not save %s: %s\n", filename, strerror(errno));
        return;
    }

    fprintf(f, "P5\n%d %d\n255\n", FRAME_WIDTH, FRAME_HEIGHT);
    fwrite(frame, 1, FRAME_WIDTH * FRAME_HEIGHT, f);
    fclose(f);

    printf("Saved %s\n", filename);
}

int main(int argc, char **argv)
{
    uintptr_t base = 0;
    int threshold = -1;
    int cal_frames = 8;
    int margin = 30;
    int min_pixels = 4;
    int continuous = 0;
    int focal_x = 500;
    int focal_y = 500;
    const char *save_file = NULL;

    int fd;
    void *map;
    uint8_t *frame;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) {
            base = (uintptr_t)strtoul(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc) {
            threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--cal-frames") == 0 && i + 1 < argc) {
            cal_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--margin") == 0 && i + 1 < argc) {
            margin = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--min-pixels") == 0 && i + 1 < argc) {
            min_pixels = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--focal-x") == 0 && i + 1 < argc) {
            focal_x = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--focal-y") == 0 && i + 1 < argc) {
            focal_y = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--continuous") == 0) {
            continuous = 1;
        } else if (strcmp(argv[i], "--save-pgm") == 0 && i + 1 < argc) {
            save_file = argv[++i];
        } else {
            printf("Usage: %s --base ADDR [--threshold N] [--focal-x N] [--focal-y N] [--continuous]\n", argv[0]);
            return 1;
        }
    }

    if (base == 0) {
        fprintf(stderr, "Missing --base address\n");
        return 1;
    }

    if (focal_x <= 0 || focal_y <= 0) {
        fprintf(stderr, "Focal lengths must be positive\n");
        return 1;
    }

    frame = malloc(FRAME_WIDTH * FRAME_HEIGHT);
    if (!frame) {
        fprintf(stderr, "Could not allocate frame buffer\n");
        return 1;
    }

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "open /dev/mem failed: %s\n", strerror(errno));
        free(frame);
        return 1;
    }

    map = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)base);
    if (map == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        close(fd);
        free(frame);
        return 1;
    }

    regs = (volatile uint32_t *)map;

    if (threshold < 0) {
        threshold = calibrate_threshold(frame, cal_frames, margin);
        if (threshold < 0)
            goto done;
    } else {
        printf("Using fixed threshold = %d\n", threshold);
    }

    do {
        int x = 0;
        int y = 0;
        int dx;
        int dy;
        int32_t angle_x;
        int32_t angle_y;
        uint64_t count = 0;
        int valid;

        if (!capture_frame(frame))
            goto done;

        if (save_file) {
            save_pgm(save_file, frame);
            save_file = NULL;
        }

        valid = find_beacon(frame, threshold, min_pixels, &x, &y, &count);

        if (valid) {
            dx = x - CENTER_X;
            dy = CENTER_Y - y;

            angle_x = cordic_atan2_deg100(dx, focal_x);
            angle_y = cordic_atan2_deg100(dy, focal_y);

            printf("Beacon found: x=%d y=%d dx=%d dy=%d bright_pixels=%" PRIu64 " ",
                   x, y, dx, dy, count);

            print_angle("angle_x", angle_x);
            printf(" ");
            print_angle("angle_y", angle_y);
            printf(" threshold=%d\n", threshold);
        } else {
            printf("Beacon not found: bright_pixels=%" PRIu64 " threshold=%d\n",
                   count, threshold);
        }

    } while (continuous);

done:
    munmap(map, MAP_SIZE);
    close(fd);
    free(frame);
    return 0;
}
