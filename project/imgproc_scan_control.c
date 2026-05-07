#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define IMGPROC_BASE 0xFF200000u
#define IMGPROC_SPAN 0x1000u

#define IMGPROC_CONTROL 0

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
        uint32_t control = regs[IMGPROC_CONTROL];
        printf("CONTROL=0x%08x DONE=%u\n", control, control & 1u);
        usleep(250000);
    }

    return 0;
}
