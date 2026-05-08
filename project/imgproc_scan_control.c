#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define IMGPROC_BASE 0xFF200000u
#define IMGPROC_SPAN 0x1000u

#define REG32(byte_offset) ((byte_offset) / 4)

#define IMG_DONE REG32(0x18u)
#define IMG_CONTROL REG32(0x1cu)

#define DONE_A (1u << 0)
#define DONE_B (1u << 1)
#define DONE_FB (1u << 2)

int main(void)
{
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

    while (1) {
        uint32_t done = regs[IMG_DONE];
        uint32_t control = regs[IMG_CONTROL];
        printf("DONE=0x%08x A=%u B=%u FB=%u CONTROL=0x%08x store=%u thA=%u thB=%u\n",
               done, (done & DONE_A) != 0, (done & DONE_B) != 0,
               (done & DONE_FB) != 0, control, control & 0x3u,
               (control >> 8) & 0xffu, (control >> 16) & 0xffu);
        usleep(250000);
    }

    return 0;
}
