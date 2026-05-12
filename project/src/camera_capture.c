#define _DEFAULT_SOURCE

#include "camera_capture.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define REG32(byte_offset) ((byte_offset) / 4u)

#define IMG_AREA_A REG32(0x00u)
#define IMG_U_A REG32(0x04u)
#define IMG_V_A REG32(0x08u)
#define IMG_AREA_B REG32(0x0cu)
#define IMG_U_B REG32(0x10u)
#define IMG_V_B REG32(0x14u)
#define IMG_DONE REG32(0x18u)
#define IMG_CONTROL REG32(0x1cu)
#define IMG_FB_INDEX REG32(0x20u)
#define IMG_FB_DATA REG32(0x24u)

#define DONE_A (1u << 0)
#define DONE_B (1u << 1)
#define DONE_FB (1u << 2)
#define DONE_MOMENTS (DONE_A | DONE_B)

#define STORE_MASK 0x3u
#define STORE_NONE 0u
#define STORE_CAMERA_A 1u
#define STORE_CAMERA_B 2u

#define THRESH_A_SHIFT 8
#define THRESH_B_SHIFT 16
#define THRESH_MASK 0xffu

#define DONE_TIMEOUT_MS 5000
#define CLEAR_TIMEOUT_MS 100
#define CLEAR_TIMEOUT_POLLS 10000000u
#define DONE_TIMEOUT_POLLS 100000000u
#define NS_PER_SEC 1000000000.0
#define NAME_LEN 256
#define PGM_HEADER_LEN 32

static const char b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int map_regs(volatile uint32_t **regs_out, int *fd_out)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *map;

    if (fd < 0) {
        perror("open /dev/mem");
        return -1;
    }

    map = mmap(NULL, DE1_LW_BRIDGE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED,
               fd, DE1_LW_BRIDGE_BASE);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    *regs_out = (volatile uint32_t *)map;
    *fd_out = fd;
    return 0;
}

static void unmap_regs(volatile uint32_t *regs, int fd)
{
    munmap((void *)regs, DE1_LW_BRIDGE_SPAN);
    close(fd);
}

static unsigned store_select(camera_select_t camera)
{
    return camera == CAMERA_B ? STORE_CAMERA_B : STORE_CAMERA_A;
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

static void finish_moments(camera_moments_t *moments)
{
    moments->cx = centroid_coord(moments->u, moments->area);
    moments->cy = centroid_coord(moments->v, moments->area);
}

static void read_moments_a(volatile uint32_t *regs, camera_moments_t *moments)
{
    moments->area = regs[IMG_AREA_A];
    moments->u = regs[IMG_U_A];
    moments->v = regs[IMG_V_A];
    finish_moments(moments);
}

static void read_moments_b(volatile uint32_t *regs, camera_moments_t *moments)
{
    moments->area = regs[IMG_AREA_B];
    moments->u = regs[IMG_U_B];
    moments->v = regs[IMG_V_B];
    finish_moments(moments);
}

static int discard_first_results(volatile uint32_t *regs)
{
    int discarded_a = 0;
    int discarded_b = 0;
    camera_moments_t unused;

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

static void set_threshold_regs(volatile uint32_t *regs, unsigned threshold_a,
                               unsigned threshold_b)
{
    regs[IMG_CONTROL] =
        (regs[IMG_CONTROL] &
         ~(STORE_MASK | (THRESH_MASK << THRESH_A_SHIFT) |
           (THRESH_MASK << THRESH_B_SHIFT))) |
        STORE_NONE | (threshold_a << THRESH_A_SHIFT) |
        (threshold_b << THRESH_B_SHIFT);
}

int camera_set_thresholds(unsigned threshold_a, unsigned threshold_b)
{
    volatile uint32_t *regs;
    int fd;

    if (threshold_a > THRESH_MASK || threshold_b > THRESH_MASK) {
        fprintf(stderr, "thresholds must be 0..255\n");
        return -1;
    }
    if (map_regs(&regs, &fd) < 0) {
        return -1;
    }

    set_threshold_regs(regs, threshold_a, threshold_b);
    printf("frame store off, thresholdA=%u thresholdB=%u CONTROL=0x%08x\n",
           threshold_a, threshold_b, regs[IMG_CONTROL]);

    clear_done(regs, DONE_MOMENTS);
    if (wait_clear(regs, DONE_MOMENTS, "initial moments") < 0 ||
        discard_first_results(regs) < 0) {
        unmap_regs(regs, fd);
        return -1;
    }

    unmap_regs(regs, fd);
    return 0;
}

static int read_moment_pair_regs(volatile uint32_t *regs,
                                 camera_moments_t *moments_a,
                                 camera_moments_t *moments_b,
                                 unsigned min_area, int *valid)
{
    int fresh_a = 0;
    int fresh_b = 0;

    for (unsigned polls = 0; !fresh_a || !fresh_b; polls++) {
        uint32_t done = regs[IMG_DONE];

        if (done & DONE_A) {
            read_moments_a(regs, moments_a);
            clear_done(regs, DONE_A);
            if (wait_clear(regs, DONE_A, "camera A moments") < 0) {
                return -1;
            }
            fresh_a = 1;
        }

        if (done & DONE_B) {
            read_moments_b(regs, moments_b);
            clear_done(regs, DONE_B);
            if (wait_clear(regs, DONE_B, "camera B moments") < 0) {
                return -1;
            }
            fresh_b = 1;
        }

        if (polls >= DONE_TIMEOUT_POLLS) {
            fprintf(stderr,
                    "moments: timeout waiting for pair; DONE=0x%08x\n",
                    regs[IMG_DONE]);
            return -1;
        }
    }

    *valid = moments_a->area >= min_area && moments_b->area >= min_area;
    return 0;
}

int camera_read_moment_pair(camera_moments_t *moments_a,
                            camera_moments_t *moments_b,
                            unsigned min_area, int *valid)
{
    volatile uint32_t *regs;
    int fd;
    int ret;

    if (map_regs(&regs, &fd) < 0) {
        return -1;
    }
    ret = read_moment_pair_regs(regs, moments_a, moments_b, min_area, valid);
    unmap_regs(regs, fd);
    return ret;
}

static int capture_frame_regs(volatile uint32_t *regs, camera_select_t camera,
                              unsigned char *pixels)
{
    uint32_t control = regs[IMG_CONTROL];
    unsigned store = store_select(camera);

    if ((control & STORE_MASK) != STORE_NONE && (regs[IMG_DONE] & DONE_FB) == 0) {
        printf("waiting for previous framebuffer capture to finish\n");
        for (unsigned ms = 0; (regs[IMG_DONE] & DONE_FB) == 0; ms++) {
            if (ms >= DONE_TIMEOUT_MS) {
                fprintf(stderr, "timeout waiting for prior frame DONE; DONE=0x%08x\n",
                        regs[IMG_DONE]);
                return -1;
            }
            usleep(1000);
        }
    }

    regs[IMG_CONTROL] = (regs[IMG_CONTROL] & ~STORE_MASK) | store;

    printf("camera %c selected; CONTROL=0x%08x DONE=0x%08x\n",
           camera_letter(camera), regs[IMG_CONTROL], regs[IMG_DONE]);
    clear_done(regs, DONE_FB);

    for (unsigned ms = 0; (regs[IMG_DONE] & DONE_FB) != 0; ms++) {
        if (ms >= CLEAR_TIMEOUT_MS) {
            fprintf(stderr, "timeout clearing frame DONE; DONE=0x%08x\n",
                    regs[IMG_DONE]);
            return -1;
        }
        usleep(1000);
    }

    printf("capture armed; waiting for DONE\n");

    for (unsigned ms = 0; (regs[IMG_DONE] & DONE_FB) == 0; ms++) {
        if (ms >= DONE_TIMEOUT_MS) {
            fprintf(stderr, "timeout waiting for frame DONE; DONE=0x%08x\n",
                    regs[IMG_DONE]);
            return -1;
        }
        usleep(1000);
    }

    regs[IMG_CONTROL] &= ~STORE_MASK;

    for (uint32_t i = 0; i < FRAME_WORDS; i++) {
        uint32_t word;

        regs[IMG_FB_INDEX] = i;
        word = regs[IMG_FB_DATA];

        pixels[4 * i + 0] = (unsigned char)((word >> 0) & 0xffu);
        pixels[4 * i + 1] = (unsigned char)((word >> 8) & 0xffu);
        pixels[4 * i + 2] = (unsigned char)((word >> 16) & 0xffu);
        pixels[4 * i + 3] = (unsigned char)((word >> 24) & 0xffu);
    }

    return 0;
}

int camera_capture_frame(camera_select_t camera, unsigned char *pixels)
{
    volatile uint32_t *regs;
    int fd;
    int ret;

    if (map_regs(&regs, &fd) < 0) {
        return -1;
    }
    ret = capture_frame_regs(regs, camera, pixels);
    unmap_regs(regs, fd);
    return ret;
}

static void base64_write(FILE *out, const unsigned char *data, size_t len)
{
    unsigned line = 0;

    for (size_t i = 0; i < len; i += 3) {
        unsigned v = (unsigned)data[i] << 16;
        int have1 = i + 1 < len;
        int have2 = i + 2 < len;

        if (have1) {
            v |= (unsigned)data[i + 1] << 8;
        }
        if (have2) {
            v |= data[i + 2];
        }

        fputc(b64[(v >> 18) & 0x3f], out);
        fputc(b64[(v >> 12) & 0x3f], out);
        fputc(have1 ? b64[(v >> 6) & 0x3f] : '=', out);
        fputc(have2 ? b64[v & 0x3f] : '=', out);

        line += 4;
        if (line >= 76) {
            fputc('\n', out);
            line = 0;
        }
    }

    if (line) {
        fputc('\n', out);
    }
}

int camera_capture_serial(camera_select_t camera, FILE *out)
{
    long run_time = (long)time(NULL);
    long pid = (long)getpid();
    char name[NAME_LEN];
    char header[PGM_HEADER_LEN];
    int name_len;
    int header_len;
    unsigned char *pixels = malloc(FRAME_PIXELS);
    unsigned char *pgm;

    if (!pixels) {
        perror("malloc pixels");
        return -1;
    }
    if (camera_capture_frame(camera, pixels) < 0) {
        free(pixels);
        return -1;
    }

    name_len = snprintf(name, sizeof(name), "frame_cam%c_%ld_%ld.pgm",
                        camera_letter(camera), run_time, pid);
    header_len = snprintf(header, sizeof(header), "P5\n%u %u\n255\n",
                          FRAME_WIDTH, FRAME_HEIGHT);
    if (name_len < 0 || (size_t)name_len >= sizeof(name) ||
        header_len < 0 || (size_t)header_len >= sizeof(header)) {
        fprintf(stderr, "serial filename/header too long\n");
        free(pixels);
        return -1;
    }

    pgm = malloc((size_t)header_len + FRAME_PIXELS);
    if (!pgm) {
        perror("malloc pgm");
        free(pixels);
        return -1;
    }

    memcpy(pgm, header, (size_t)header_len);
    memcpy(pgm + header_len, pixels, FRAME_PIXELS);

    fprintf(out, "BEGIN_FRAME 0 %s\n", name);
    base64_write(out, pgm, (size_t)header_len + FRAME_PIXELS);
    fprintf(out, "END_FRAME 0\n");
    fflush(out);

    free(pgm);
    free(pixels);
    return 0;
}

int camera_debug_stream(unsigned print_every)
{
    volatile uint32_t *regs;
    int fd;
    struct timespec last_print;
    int have_last_print = 0;
    int fresh_a = 0;
    int fresh_b = 0;
    unsigned sample = 0;
    camera_moments_t moments_a = {0, 0, 0, 0.0, 0.0};
    camera_moments_t moments_b = {0, 0, 0, 0.0, 0.0};
    uint32_t paired_done_snapshot = 0;
    unsigned idle_polls = 0;

    if (print_every == 0) {
        print_every = 1;
    }
    if (map_regs(&regs, &fd) < 0) {
        return -1;
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

        if (sample % print_every == 0) {
            struct timespec now;
            double hz = 0.0;

            clock_gettime(CLOCK_MONOTONIC, &now);
            if (have_last_print) {
                double dt = elapsed_seconds(&last_print, &now);
                if (dt > 0.0) {
                    hz = (double)print_every / dt;
                }
            }
            last_print = now;
            have_last_print = 1;

            printf("%u,%u,%u,%.2f,%.2f,%u,%u,%u,%.2f,%.2f,0x%08x,%.2f\n",
                   moments_a.area, moments_a.u, moments_a.v,
                   moments_a.cx, moments_a.cy,
                   moments_b.area, moments_b.u, moments_b.v,
                   moments_b.cx, moments_b.cy,
                   paired_done_snapshot, hz);
            fflush(stdout);
        }

        fresh_a = 0;
        fresh_b = 0;
        paired_done_snapshot = 0;
    }

    unmap_regs(regs, fd);
    return -1;
}

int camera_print_status(void)
{
    volatile uint32_t *regs;
    int fd;
    uint32_t done;
    uint32_t control;

    if (map_regs(&regs, &fd) < 0) {
        return -1;
    }

    done = regs[IMG_DONE];
    control = regs[IMG_CONTROL];
    printf("DONE=0x%08x A=%u B=%u FB=%u CONTROL=0x%08x store=%u thA=%u thB=%u\n",
           done, (done & DONE_A) != 0, (done & DONE_B) != 0,
           (done & DONE_FB) != 0, control, control & STORE_MASK,
           (control >> THRESH_A_SHIFT) & THRESH_MASK,
           (control >> THRESH_B_SHIFT) & THRESH_MASK);

    unmap_regs(regs, fd);
    return 0;
}
