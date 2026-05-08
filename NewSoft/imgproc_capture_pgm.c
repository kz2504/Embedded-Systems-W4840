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

static char camera_name(unsigned store_select)
{
    return store_select == STORE_CAMERA_B ? 'B' : 'A';
}

static int parse_camera(const char *text, unsigned *store_select)
{
    if (strcmp(text, "A") == 0 || strcmp(text, "a") == 0 || strcmp(text, "0") == 0) {
        *store_select = STORE_CAMERA_A;
        return 0;
    }

    if (strcmp(text, "B") == 0 || strcmp(text, "b") == 0 || strcmp(text, "1") == 0) {
        *store_select = STORE_CAMERA_B;
        return 0;
    }

    return -1;
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

    regs[IMG_CONTROL] = (regs[IMG_CONTROL] & ~STORE_MASK) | store_select;

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

int main(int argc, char **argv)
{
    const char *out_dir = ".";
    long run_time = (long)time(NULL);
    long pid = (long)getpid();
    unsigned store_select = STORE_CAMERA_A;
    int argi = 1;

    unsigned char *pixels = malloc(PIXELS);

    if (!pixels) {
        perror("malloc");
        return 1;
    }

    if (argi < argc && parse_camera(argv[argi], &store_select) == 0) {
        argi++;
    }

    if (argi < argc) {
        out_dir = argv[argi];
        argi++;
    }

    if (argi != argc) {
        fprintf(stderr, "usage: %s [A|B] [output_dir]\n", argv[0]);
        free(pixels);
        return 2;
    }

    if (mkdir(out_dir, 0777) < 0) {
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

    if (name_len < 0 || (size_t)name_len >= sizeof(path)) {
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

    printf("capture done; writing %s\n", path);

    if (write_pgm_file(path, pixels) < 0) {
        munmap(map, IMGPROC_SPAN);
        close(fd);
        free(pixels);
        return 1;
    }

    munmap(map, IMGPROC_SPAN);
    close(fd);
    free(pixels);

    printf("wrote %d x %d grayscale PGM: %s\n", WIDTH, HEIGHT, path);
    return 0;
}

