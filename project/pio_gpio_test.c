#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BRIDGE_BASE 0xFF200000u
#define LW_BRIDGE_SPAN 0x1000u

#define GPIO1_UPPER_OFFSET 0x20u
#define PIO_DATA 0x00u
#define PIO_DIR 0x04u

#define GPIO_BITS 0xFFu
#define HALF_PERIOD_US 1000000

static volatile sig_atomic_t stop;

static void on_signal(int sig)
{
    (void)sig;
    stop = 1;
}

static void usage(const char *name)
{
    fprintf(stderr, "usage: %s [cycles]\n", name);
    fprintf(stderr, "  cycles: 0 means loop until Ctrl-C (default: 4)\n");
}

int main(int argc, char **argv)
{
    unsigned cycles = 4;
    int fd;
    void *map;
    volatile uint32_t *data;
    volatile uint32_t *dir;
    uint32_t old_data;
    uint32_t old_dir;

    if (argc > 2) {
        usage(argv[0]);
        return 2;
    }

    if (argc == 2) {
        char *end = NULL;
        errno = 0;
        unsigned long value = strtoul(argv[1], &end, 0);
        if (errno || *end || value > 1000000ul) {
            usage(argv[0]);
            return 2;
        }
        cycles = (unsigned)value;
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    map = mmap(NULL, LW_BRIDGE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
               LW_BRIDGE_BASE);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    data = (volatile uint32_t *)((uint8_t *)map + GPIO1_UPPER_OFFSET + PIO_DATA);
    dir = (volatile uint32_t *)((uint8_t *)map + GPIO1_UPPER_OFFSET + PIO_DIR);

    old_data = *data;
    old_dir = *dir;

    *data = old_data & ~GPIO_BITS;
    *dir = old_dir | GPIO_BITS;

    printf("Toggling GPIO_1[28:35] at 0.5 Hz. Ctrl-C to stop.\n");

    for (unsigned c = 0; !stop && (cycles == 0 || c < cycles); c++) {
        *data = (*data & ~GPIO_BITS) | GPIO_BITS;
        printf("all high\n");
        usleep(HALF_PERIOD_US);

        *data &= ~GPIO_BITS;
        printf("all low\n");
        usleep(HALF_PERIOD_US);
    }

    *data = (*data & ~GPIO_BITS) | (old_data & GPIO_BITS);
    *dir = (*dir & ~GPIO_BITS) | (old_dir & GPIO_BITS);

    munmap(map, LW_BRIDGE_SPAN);
    close(fd);
    return 0;
}
