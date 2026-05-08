#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
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
#define STORE_MASK 0x3u
#define STORE_NONE 0u
#define THRESH_A_SHIFT 8
#define THRESH_B_SHIFT 16
#define THRESH_MASK 0xffu

#define CLEAR_TIMEOUT_MS 100
#define DONE_TIMEOUT_MS 1000
#define FRAME_US 33333

static int wait_clear(volatile uint32_t *regs, uint32_t bits, const char *name)
{
    for (unsigned ms = 0; (regs[IMG_DONE] & bits) != 0; ms++) {
        if (ms >= CLEAR_TIMEOUT_MS) {
            fprintf(stderr, "%s: timeout clearing DONE; DONE=0x%08x\n",
                    name, regs[IMG_DONE]);
            return -1;
        }
        usleep(1000);
    }
    return 0;
}

static int wait_done(volatile uint32_t *regs, uint32_t bits, const char *name)
{
    for (unsigned ms = 0; (regs[IMG_DONE] & bits) != bits; ms++) {
        if (ms >= DONE_TIMEOUT_MS) {
            fprintf(stderr, "%s: timeout waiting for DONE; DONE=0x%08x\n",
                    name, regs[IMG_DONE]);
            return -1;
        }
        usleep(1000);
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

    regs[IMG_CONTROL] =
        (regs[IMG_CONTROL] &
         ~(STORE_MASK | (THRESH_MASK << THRESH_A_SHIFT) |
           (THRESH_MASK << THRESH_B_SHIFT))) |
        STORE_NONE | (threshold_a << THRESH_A_SHIFT) |
        (threshold_b << THRESH_B_SHIFT);

    printf("frame store off, thresholdA=%u thresholdB=%u CONTROL=0x%08x\n",
           threshold_a, threshold_b, regs[IMG_CONTROL]);

    printf("areaA,uA,vA,areaB,uB,vB,done\n");

    while (1) {
        uint32_t area_a;
        uint32_t u_a;
        uint32_t v_a;
        uint32_t area_b;
        uint32_t u_b;
        uint32_t v_b;

        regs[IMG_DONE] = ~DONE_A;
        if (wait_clear(regs, DONE_A, "camera A") < 0 ||
            wait_done(regs, DONE_A, "camera A") < 0) {
            break;
        }
        area_a = regs[IMG_AREA_A];
        u_a = regs[IMG_U_A];
        v_a = regs[IMG_V_A];

        regs[IMG_DONE] = ~DONE_B;
        if (wait_clear(regs, DONE_B, "camera B") < 0 ||
            wait_done(regs, DONE_B, "camera B") < 0) {
            break;
        }
        area_b = regs[IMG_AREA_B];
        u_b = regs[IMG_U_B];
        v_b = regs[IMG_V_B];

        printf("%u,%u,%u,%u,%u,%u,0x%08x\n",
               area_a, u_a, v_a, area_b, u_b, v_b, regs[IMG_DONE]);
        fflush(stdout);

        usleep(FRAME_US);
    }

    munmap(map, IMGPROC_SPAN);
    close(fd);
    return 1;
}
