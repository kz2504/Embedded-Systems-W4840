#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
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
#define NS_PER_SEC 1000000000.0
#define PRINT_EVERY 30

typedef struct {
    uint32_t area;
    uint32_t u;
    uint32_t v;
} moments_t;

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

static void read_moments_a(volatile uint32_t *regs, moments_t *moments)
{
    moments->area = regs[IMG_AREA_A];
    moments->u = regs[IMG_U_A];
    moments->v = regs[IMG_V_A];
}

static void read_moments_b(volatile uint32_t *regs, moments_t *moments)
{
    moments->area = regs[IMG_AREA_B];
    moments->u = regs[IMG_U_B];
    moments->v = regs[IMG_V_B];
}

static int discard_first_results(volatile uint32_t *regs)
{
    int discarded_a = 0;
    int discarded_b = 0;
    moments_t unused;

    for (unsigned polls = 0; !discarded_a || !discarded_b; polls++) {
        uint32_t done = regs[IMG_DONE];

        if (!discarded_a && (done & DONE_A)) {
            read_moments_a(regs, &unused);
            clear_done(regs, DONE_A);
            if (wait_clear(regs, DONE_A, "initial camera A moments") < 0) {
                return -1;
            }
            discarded_a = 1;
        }

        if (!discarded_b && (done & DONE_B)) {
            read_moments_b(regs, &unused);
            clear_done(regs, DONE_B);
            if (wait_clear(regs, DONE_B, "initial camera B moments") < 0) {
                return -1;
            }
            discarded_b = 1;
        }

        if (polls >= DONE_TIMEOUT_POLLS) {
            fprintf(stderr,
                    "initial moments: timeout waiting for first results; DONE=0x%08x\n",
                    regs[IMG_DONE]);
            return -1;
        }
    }

    return 0;
}

static int parse_threshold(const char *text, unsigned *threshold)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || *end || value > THRESH_MASK) {
        return -1;
    }

    *threshold = (unsigned)value;
    return 0;
}

static void usage(const char *name)
{
    fprintf(stderr, "usage: %s [threshold_a [threshold_b]]\n", name);
    fprintf(stderr, "  thresholds are 0..255; one value applies to both cameras\n");
}

static double elapsed_seconds(const struct timespec *start,
                              const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / NS_PER_SEC;
}

static double centroid_coord(uint32_t moment, uint32_t area)
{
    if (area == 0) {
        return -1.0;
    }
    return (double)moment / (double)area;
}

int main(int argc, char **argv)
{
    unsigned threshold_a = 0;
    unsigned threshold_b = 0;

    if (argc > 3) {
        usage(argv[0]);
        return 2;
    }
    if (argc >= 2 && parse_threshold(argv[1], &threshold_a) < 0) {
        usage(argv[0]);
        return 2;
    }
    threshold_b = threshold_a;
    if (argc == 3 && parse_threshold(argv[2], &threshold_b) < 0) {
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
    struct timespec last_print;
    int have_last_print = 0;
    int fresh_a = 0;
    int fresh_b = 0;
    unsigned sample = 0;
    moments_t moments_a = {0, 0, 0};
    moments_t moments_b = {0, 0, 0};
    uint32_t paired_done_snapshot = 0;
    unsigned idle_polls = 0;

    regs[IMG_CONTROL] =
        (regs[IMG_CONTROL] &
         ~(STORE_MASK | (THRESH_MASK << THRESH_A_SHIFT) |
           (THRESH_MASK << THRESH_B_SHIFT))) |
        STORE_NONE | (threshold_a << THRESH_A_SHIFT) |
        (threshold_b << THRESH_B_SHIFT);

    printf("frame store off, thresholdA=%u thresholdB=%u CONTROL=0x%08x\n",
           threshold_a, threshold_b, regs[IMG_CONTROL]);

    clear_done(regs, DONE_MOMENTS);
    if (wait_clear(regs, DONE_MOMENTS, "initial moments") < 0 ||
        discard_first_results(regs) < 0) {
        munmap(map, IMGPROC_SPAN);
        close(fd);
        return 1;
    }

    printf("areaA,uA,vA,cxA,cyA,areaB,uB,vB,cxB,cyB,done,hz\n");

    while (1) {
        uint32_t done = regs[IMG_DONE];

        if ((done & DONE_MOMENTS) == 0) {
            idle_polls++;
            if (idle_polls >= DONE_TIMEOUT_POLLS) {
                fprintf(stderr,
                        "moments: timeout waiting for DONE; DONE=0x%08x\n",
                        regs[IMG_DONE]);
                break;
            }
            continue;
        }
        idle_polls = 0;

        if (done & DONE_A) {
            read_moments_a(regs, &moments_a);
            clear_done(regs, DONE_A);
            if (wait_clear(regs, DONE_A, "camera A moments") < 0) {
                break;
            }
            fresh_a = 1;
            paired_done_snapshot |= DONE_A;
        }

        if (done & DONE_B) {
            read_moments_b(regs, &moments_b);
            clear_done(regs, DONE_B);
            if (wait_clear(regs, DONE_B, "camera B moments") < 0) {
                break;
            }
            fresh_b = 1;
            paired_done_snapshot |= DONE_B;
        }

        if (!fresh_a || !fresh_b) {
            continue;
        }

        sample++;

        if (sample % PRINT_EVERY == 0) {
            struct timespec now;
            double hz = 0.0;
            double cx_a = centroid_coord(moments_a.u, moments_a.area);
            double cy_a = centroid_coord(moments_a.v, moments_a.area);
            double cx_b = centroid_coord(moments_b.u, moments_b.area);
            double cy_b = centroid_coord(moments_b.v, moments_b.area);

            clock_gettime(CLOCK_MONOTONIC, &now);
            if (have_last_print) {
                double dt = elapsed_seconds(&last_print, &now);
                if (dt > 0.0) {
                    hz = (double)PRINT_EVERY / dt;
                }
            }
            last_print = now;
            have_last_print = 1;

            printf("%u,%u,%u,%.2f,%.2f,%u,%u,%u,%.2f,%.2f,0x%08x,%.2f\n",
                   moments_a.area, moments_a.u, moments_a.v,
                   cx_a, cy_a,
                   moments_b.area, moments_b.u, moments_b.v,
                   cx_b, cy_b,
                   paired_done_snapshot, hz);
            fflush(stdout);
        }

        fresh_a = 0;
        fresh_b = 0;
        paired_done_snapshot = 0;
    }

    munmap(map, IMGPROC_SPAN);
    close(fd);
    return 1;
}
