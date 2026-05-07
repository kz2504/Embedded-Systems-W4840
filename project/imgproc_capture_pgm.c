#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
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
#define FRAME_COUNT 10
#define NAME_LEN 256

int main(int argc, char **argv)
{
    const char *prefix = "frame";

    if (argc > 2) {
        fprintf(stderr, "usage: %s [output_prefix]\n", argv[0]);
        return 2;
    }
    if (argc == 2) {
        prefix = argv[1];
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

    for (unsigned frame = 0; frame < FRAME_COUNT; frame++) {
        char path[NAME_LEN];
        int name_len = snprintf(path, sizeof(path), "%s_%03u.pgm", prefix, frame);

        if (name_len < 0 || (size_t)name_len >= sizeof(path)) {
            fprintf(stderr, "output filename too long\n");
            munmap(map, IMGPROC_SPAN);
            close(fd);
            return 1;
        }

        printf("frame %u: CONTROL before arm: 0x%08x\n", frame,
               regs[IMGPROC_CONTROL]);
        regs[IMGPROC_CONTROL] = 0;
        for (unsigned ms = 0; (regs[IMGPROC_CONTROL] & 1u) != 0; ms++) {
            if (ms >= CLEAR_TIMEOUT_MS) {
                fprintf(stderr, "frame %u: timeout clearing DONE; CONTROL=0x%08x\n",
                        frame, regs[IMGPROC_CONTROL]);
                munmap(map, IMGPROC_SPAN);
                close(fd);
                return 1;
            }
            usleep(1000);
        }
        printf("frame %u: capture armed; waiting for DONE\n", frame);

        for (unsigned ms = 0; (regs[IMGPROC_CONTROL] & 1u) == 0; ms++) {
            if (ms >= DONE_TIMEOUT_MS) {
                fprintf(stderr, "frame %u: timeout waiting for DONE; CONTROL=0x%08x\n",
                        frame, regs[IMGPROC_CONTROL]);
                munmap(map, IMGPROC_SPAN);
                close(fd);
                return 1;
            }
            usleep(1000);
        }

        printf("frame %u: capture done; writing %s\n", frame, path);

        FILE *out = fopen(path, "wb");
        if (!out) {
            perror("fopen output");
            munmap(map, IMGPROC_SPAN);
            close(fd);
            return 1;
        }

        fprintf(out, "P5\n%d %d\n255\n", WIDTH, HEIGHT);

        for (uint32_t i = 0; i < WORDS; i++) {
            uint32_t word;
            unsigned char pixels[4];

            regs[IMGPROC_INDEX] = i;
            word = regs[IMGPROC_DATA];

            pixels[0] = (unsigned char)((word >> 0) & 0xffu);
            pixels[1] = (unsigned char)((word >> 8) & 0xffu);
            pixels[2] = (unsigned char)((word >> 16) & 0xffu);
            pixels[3] = (unsigned char)((word >> 24) & 0xffu);

            if (fwrite(pixels, 1, sizeof(pixels), out) != sizeof(pixels)) {
                perror("write output");
                fclose(out);
                munmap(map, IMGPROC_SPAN);
                close(fd);
                return 1;
            }
        }

        fclose(out);
    }

    munmap(map, IMGPROC_SPAN);
    close(fd);

    printf("wrote %u frames as %s_000.pgm .. %s_%03u.pgm\n",
           FRAME_COUNT, prefix, prefix, FRAME_COUNT - 1);
    return 0;
}
