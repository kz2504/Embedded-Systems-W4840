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

#define REG32(byte_offset) ((byte_offset) / 4)

#define IMG_DONE REG32(0x18u)
#define IMG_CONTROL REG32(0x1cu)
#define IMG_FB_INDEX REG32(0x20u)
#define IMG_FB_DATA REG32(0x24u)

#define DONE_FB (1u << 2)
#define STORE_MASK 0x3u
#define STORE_NONE 0u
#define STORE_CAMERA_A 1u
#define STORE_CAMERA_B 2u

#define WIDTH 640
#define HEIGHT 480
#define PIXELS (WIDTH * HEIGHT)
#define WORDS (PIXELS / 4)
#define DONE_TIMEOUT_MS 5000
#define CLEAR_TIMEOUT_MS 100
#define NAME_LEN 256
#define PGM_HEADER_LEN 32

static const char b64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char camera_name(unsigned store_select)
{
    return store_select == STORE_CAMERA_B ? 'B' : 'A';
}

static int capture_frame(volatile uint32_t *regs, unsigned store_select,
                         unsigned char *pixels)
{
    uint32_t control = regs[IMG_CONTROL];

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

    control = (regs[IMG_CONTROL] & ~STORE_MASK) | store_select;
    regs[IMG_CONTROL] = control;

    printf("camera %c selected; CONTROL=0x%08x DONE=0x%08x\n",
           camera_name(store_select), regs[IMG_CONTROL], regs[IMG_DONE]);
    regs[IMG_DONE] = ~DONE_FB;

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

    regs[IMG_CONTROL] = regs[IMG_CONTROL] & ~STORE_MASK;

    for (uint32_t i = 0; i < WORDS; i++) {
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

static int serial_write_pgm(unsigned store_select, long run_time, long pid,
                            const unsigned char *pixels)
{
    char name[NAME_LEN];
    char header[PGM_HEADER_LEN];
    unsigned char *pgm;
    int name_len = snprintf(name, sizeof(name), "frame_cam%c_%ld_%ld.pgm",
                            camera_name(store_select), run_time, pid);
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

    printf("BEGIN_FRAME 0 %s\n", name);
    base64_write(stdout, pgm, (size_t)header_len + PIXELS);
    printf("END_FRAME 0\n");
    fflush(stdout);
    free(pgm);
    return 0;
}

static int parse_camera(const char *text, unsigned *store_select)
{
    if (strcmp(text, "A") == 0 || strcmp(text, "a") == 0 ||
        strcmp(text, "0") == 0) {
        *store_select = STORE_CAMERA_A;
        return 0;
    }
    if (strcmp(text, "B") == 0 || strcmp(text, "b") == 0 ||
        strcmp(text, "1") == 0) {
        *store_select = STORE_CAMERA_B;
        return 0;
    }
    return -1;
}

static void usage(const char *name)
{
    fprintf(stderr, "usage: %s [A|B] [output_dir|--serial]\n", name);
}

int main(int argc, char **argv)
{
    const char *out_dir = ".";
    long run_time = (long)time(NULL);
    long pid = (long)getpid();
    int serial_mode = 0;
    unsigned store_select = STORE_CAMERA_A;
    int argi = 1;
    unsigned char *pixels = malloc(PIXELS);

    if (!pixels) {
        perror("malloc");
        return 1;
    }

    if (argc > 3) {
        usage(argv[0]);
        free(pixels);
        return 2;
    }

    if (argi < argc && parse_camera(argv[argi], &store_select) == 0) {
        argi++;
    }

    if (argi < argc) {
        if (strcmp(argv[argi], "--serial") == 0) {
            serial_mode = 1;
        } else {
            out_dir = argv[argi];
        }
        argi++;
    }

    if (argi != argc) {
        usage(argv[0]);
        free(pixels);
        return 2;
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

    char path[NAME_LEN];
    int name_len = snprintf(path, sizeof(path), "%s/frame_cam%c_%ld_%ld.pgm",
                            out_dir, camera_name(store_select), run_time, pid);

    if (!serial_mode && (name_len < 0 || (size_t)name_len >= sizeof(path))) {
        fprintf(stderr, "output filename too long\n");
        munmap(map, IMGPROC_SPAN);
        close(fd);
        free(pixels);
        return 1;
    }

    if (capture_frame(regs, store_select, pixels) < 0) {
        munmap(map, IMGPROC_SPAN);
        close(fd);
        free(pixels);
        return 1;
    }

    if (serial_mode) {
        if (serial_write_pgm(store_select, run_time, pid, pixels) < 0) {
            munmap(map, IMGPROC_SPAN);
            close(fd);
            free(pixels);
            return 1;
        }
    } else {
        printf("capture done; writing %s\n", path);
        if (write_pgm_file(path, pixels) < 0) {
            munmap(map, IMGPROC_SPAN);
            close(fd);
            free(pixels);
            return 1;
        }
    }

    munmap(map, IMGPROC_SPAN);
    close(fd);
    free(pixels);

    if (!serial_mode) {
        printf("wrote %d x %d grayscale PGM: %s\n", WIDTH, HEIGHT, path);
    }
    return 0;
}
