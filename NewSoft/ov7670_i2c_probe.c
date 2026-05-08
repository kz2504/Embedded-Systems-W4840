#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define GPIO1_I2C_BASE 0xFF200040u
#define MAP_SPAN 0x1000u

int main(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);

    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    void *map = mmap(NULL, MAP_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd, GPIO1_I2C_BASE);

    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    volatile uint32_t *pio = (volatile uint32_t *)map;

    printf("gpio1_i2c PIO mapped at 0x%08x, data=0x%08x\n",
           GPIO1_I2C_BASE, pio[0]);

    printf("This checks the PIO map only. Use your full OV7670 probe if camera I2C setup is needed.\n");

    munmap(map, MAP_SPAN);
    close(fd);
    return 0;
}

