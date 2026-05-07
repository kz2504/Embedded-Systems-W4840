#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define IMGPROC_BASE 0xFF200000u
#define IMGPROC_SPAN 0x1000u

#define IMGPROC_CONTROL 0
#define IMGPROC_INDEX 1
#define IMGPROC_DATA 2

#define WIDTH 320
#define HEIGHT 240
#define PIXELS (WIDTH * HEIGHT)
#define WORDS (PIXELS / 4)
#define DONE_TIMEOUT_MS 5000
#define CLEAR_TIMEOUT_MS 100
#define FRAME_COUNT 1
#define NAME_LEN 256
#define PGM_HEADER_LEN 32

static const char b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int capture_frame(volatile uint32_t *regs, unsigned frame,
                         unsigned char *pixels)
{
    printf("frame %u: CONTROL before arm: 0x%08x\n", frame,
           regs[IMGPROC_CONTROL]);
    regs[IMGPROC_CONTROL] = 0;

    for (unsigned ms = 0; (regs[IMGPROC_CONTROL] & 1u) != 0; ms++) {
        if (ms >= CLEAR_TIMEOUT_MS) {
            fprintf(stderr, "frame %u: timeout clearing DONE; CONTROL=0x%08x\n",
                    frame, regs[IMGPROC_CONTROL]);
            return -1;
        }
        usleep(1000);
    }

    printf("frame %u: capture armed; waiting for DONE\n", frame);

    for (unsigned ms = 0; (regs[IMGPROC_CONTROL] & 1u) == 0; ms++) {
        if (ms >= DONE_TIMEOUT_MS) {
            fprintf(stderr, "frame %u: timeout waiting for DONE; CONTROL=0x%08x\n",
                    frame, regs[IMGPROC_CONTROL]);
            return -1;
        }
        usleep(1000);
    }

    for (uint32_t i = 0; i < WORDS; i++) {
        uint32_t word;

        regs[IMGPROC_INDEX] = i;
        word = regs[IMGPROC_DATA];

        pixels[4 * i + 0] = (unsigned char)((word >> 0) & 0xffu);
        pixels[4 * i + 1] = (unsigned char)((word >> 8) & 0xffu);
        pixels[4 * i + 2] = (unsigned char)((word >> 16) & 0xffu);
        pixels[4 * i + 3] = (unsigned char)((word >> 24) & 0xffu);
    }

    return 0;
}

static int write_pgm_file(const char *path, const unsigned char *pixels)
{
    FILE *out = fopen(path, "wb");
    if (!out) {
        perror("fopen output");
        return -1;
    }

    fprintf(out, "P5\n%d %d\n255\n", WIDTH, HEIGHT);

    if (fwrite(pixels, 1, PIXELS, out) != PIXELS) {
        perror("write output");
        fclose(out);
        return -1;
    }

    fclose(out);
    return 0;
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

static int serial_write_pgm(unsigned frame, long run_time, long pid,
                            const unsigned char *pixels)
{
    char name[NAME_LEN];
    char header[PGM_HEADER_LEN];
    unsigned char *pgm;
    int name_len = snprintf(name, sizeof(name), "frame_%ld_%ld_%03u.pgm",
                            run_time, pid, frame);
    int header_len = snprintf(header, sizeof(header), "P5\n%d %d\n255\n",
                              WIDTH, HEIGHT);

    if (name_len < 0 || (size_t)name_len >= sizeof(name) ||
        header_len < 0 || (size_t)header_len >= sizeof(header)) {
        fprintf(stderr, "serial filename/header too long\n");
        return -1;
    }

    pgm = malloc((size_t)header_len + PIXELS);
    if (!pgm) {
        perror("malloc pgm");
        return -1;
    }
    memcpy(pgm, header, (size_t)header_len);
    memcpy(pgm + header_len, pixels, PIXELS);

    printf("BEGIN_FRAME %u %s\n", frame, name);
    base64_write(stdout, pgm, (size_t)header_len + PIXELS);
    printf("END_FRAME %u\n", frame);
    fflush(stdout);
    free(pgm);
    return 0;
}

int main(int argc, char **argv)
{
    const char *out_dir = ".";
    long run_time = (long)time(NULL);
    long pid = (long)getpid();
    int serial_mode = 0;
    unsigned char *pixels = malloc(PIXELS);

    if (!pixels) {
        perror("malloc");
        return 1;
    }

    if (argc > 2) {
        fprintf(stderr, "usage: %s [output_dir|--serial]\n", argv[0]);
        free(pixels);
        return 2;
    }
    if (argc == 2) {
        if (strcmp(argv[1], "--serial") == 0) {
            serial_mode = 1;
        } else {
            out_dir = argv[1];
        }
    }

    if (!serial_mode && mkdir(out_dir, 0777) < 0) {
        struct stat st;
        if (stat(out_dir, &st) < 0 || !S_ISDIR(st.st_mode)) {
            perror("mkdir output_dir");
            free(pixels);
            return 1;
        }
    }

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        free(pixels);
        return 1;
    }

    void *map = mmap(NULL, IMGPROC_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd, IMGPROC_BASE);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        free(pixels);
        return 1;
    }

    volatile uint32_t *regs = (volatile uint32_t *)map;

    if (serial_mode) {
        printf("BEGIN_DE1_FRAMES %u %d %d\n", FRAME_COUNT, WIDTH, HEIGHT);
    }

    for (unsigned frame = 0; frame < FRAME_COUNT; frame++) {
        char path[NAME_LEN];
        int name_len = snprintf(path, sizeof(path), "%s/frame_%ld_%ld_%03u.pgm",
                                out_dir, run_time, pid, frame);

        if (!serial_mode && (name_len < 0 || (size_t)name_len >= sizeof(path))) {
            fprintf(stderr, "output filename too long\n");
            munmap(map, IMGPROC_SPAN);
            close(fd);
            free(pixels);
            return 1;
        }

        if (capture_frame(regs, frame, pixels) < 0) {
            munmap(map, IMGPROC_SPAN);
            close(fd);
            free(pixels);
            return 1;
        }

        if (serial_mode) {
            if (serial_write_pgm(frame, run_time, pid, pixels) < 0) {
                munmap(map, IMGPROC_SPAN);
                close(fd);
                free(pixels);
                return 1;
            }
        } else {
            printf("frame %u: capture done; writing %s\n", frame, path);
            if (write_pgm_file(path, pixels) < 0) {
                munmap(map, IMGPROC_SPAN);
                close(fd);
                free(pixels);
                return 1;
            }
        }
    }

    if (serial_mode) {
        printf("END_DE1_FRAMES\n");
    }

    munmap(map, IMGPROC_SPAN);
    close(fd);
    free(pixels);

    if (!serial_mode) {
        printf("wrote %u frames as %s/frame_%ld_%ld_000.pgm .. "
               "%s/frame_%ld_%ld_%03u.pgm\n",
               FRAME_COUNT, out_dir, run_time, pid, out_dir, run_time, pid,
               FRAME_COUNT - 1);
    }
    return 0;
}
