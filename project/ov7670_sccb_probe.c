/*
 * ov7670_sccb_probe.c
 *
 * Standalone userspace SCCB/I2C probe for an OV7670 connected to the
 * DE1-SoC HPS GPIO1 port.
 *
 * Compile on the DE1-SoC Linux side with:
 *   gcc ov7670_sccb_probe.c -o ov7670_sccb_probe -pthread
 *
 * Run as root because /dev/mem is used:
 *   sudo ./ov7670_sccb_probe
 *
 * Pin mapping on HPS GPIO1:
 *   GPIO1[26] = SIOC / SCL
 *   GPIO1[27] = SIOD / SDA
 *   GPIO1[28] = PWDN
 *   GPIO1[29] = RST / RET
 *   GPIO1[30] = slow test XCLK output
 *
 * SCL and SDA are used as open-drain signals:
 *   drive low    -> write output latch low, then configure as output
 *   release high -> configure as input/high-Z, external 4.7k pullup supplies high
 *
 * Never actively drive SCL or SDA high.
 */

#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * DE1-SoC / Cyclone V HPS GPIO1 base address and register offsets.
 * Adjust these if your board support package or memory map differs.
 *
 * Common Cyclone V HPS GPIO bases:
 *   GPIO0 = 0xFF708000
 *   GPIO1 = 0xFF709000
 *   GPIO2 = 0xFF70A000
 */
#define GPIO1_BASE_PHYS      0xFF709000UL
#define GPIO_MAP_SIZE        0x1000

/* GPIO register offsets for port A. */
#define GPIO_SWPORTA_DR      0x00    /* data output latch */
#define GPIO_SWPORTA_DDR     0x04    /* direction: 1 = output, 0 = input */
#define GPIO_EXT_PORTA       0x50    /* sampled input value */

/* GPIO1 pin numbers. */
#define PIN_SIOC             26
#define PIN_SIOD             27
#define PIN_PWDN             28
#define PIN_RST              29
#define PIN_XCLK             30

/* OV7670 SCCB addresses. */
#define OV7670_ADDR_WR       0x42
#define OV7670_ADDR_RD       0x43
#define OV7670_ADDR_7BIT     0x21
#define OV7670_REG_PID       0x0A
#define OV7670_REG_VER       0x0B

/*
 * Delays are intentionally slow. Linux userspace timing is not precise, but
 * this is enough for a simple register probe with a GPIO-generated test XCLK.
 */
#define SCCB_DELAY_US        10
#define XCLK_HALF_PERIOD_US  50      /* about 10 kHz square wave */
#define RESET_LOW_US         50000
#define AFTER_RESET_US       100000
#define STARTUP_XCLK_US      200000

static volatile uint32_t *gpio1;
static int mem_fd = -1;
static pthread_t xclk_thread;
static volatile int xclk_running;
static pthread_mutex_t gpio_lock = PTHREAD_MUTEX_INITIALIZER;

static inline uint32_t pin_mask(unsigned pin)
{
    return 1u << pin;
}

static inline uint32_t gpio_read_reg(unsigned offset)
{
    return gpio1[offset / sizeof(uint32_t)];
}

static inline void gpio_write_reg(unsigned offset, uint32_t value)
{
    gpio1[offset / sizeof(uint32_t)] = value;
}

void gpio_set_output(unsigned pin)
{
    pthread_mutex_lock(&gpio_lock);
    gpio_write_reg(GPIO_SWPORTA_DDR,
                   gpio_read_reg(GPIO_SWPORTA_DDR) | pin_mask(pin));
    pthread_mutex_unlock(&gpio_lock);
}

void gpio_set_input(unsigned pin)
{
    pthread_mutex_lock(&gpio_lock);
    gpio_write_reg(GPIO_SWPORTA_DDR,
                   gpio_read_reg(GPIO_SWPORTA_DDR) & ~pin_mask(pin));
    pthread_mutex_unlock(&gpio_lock);
}

void gpio_write(unsigned pin, int value)
{
    pthread_mutex_lock(&gpio_lock);
    uint32_t dr = gpio_read_reg(GPIO_SWPORTA_DR);

    if (value)
        dr |= pin_mask(pin);
    else
        dr &= ~pin_mask(pin);

    gpio_write_reg(GPIO_SWPORTA_DR, dr);
    pthread_mutex_unlock(&gpio_lock);
}

int gpio_read(unsigned pin)
{
    uint32_t value;

    pthread_mutex_lock(&gpio_lock);
    value = gpio_read_reg(GPIO_EXT_PORTA);
    pthread_mutex_unlock(&gpio_lock);

    return (value & pin_mask(pin)) ? 1 : 0;
}

void sccb_delay(void)
{
    usleep(SCCB_DELAY_US);
}

static void scl_drive_low(void)
{
    gpio_write(PIN_SIOC, 0);
    gpio_set_output(PIN_SIOC);
}

static void scl_release(void)
{
    gpio_set_input(PIN_SIOC);
}

static void sda_drive_low(void)
{
    gpio_write(PIN_SIOD, 0);
    gpio_set_output(PIN_SIOD);
}

static void sda_release(void)
{
    gpio_set_input(PIN_SIOD);
}

void sccb_start(void)
{
    sda_release();
    scl_release();
    sccb_delay();

    sda_drive_low();
    sccb_delay();

    scl_drive_low();
    sccb_delay();
}

void sccb_stop(void)
{
    sda_drive_low();
    sccb_delay();

    scl_release();
    sccb_delay();

    sda_release();
    sccb_delay();
}

int sccb_write_byte(uint8_t byte)
{
    int bit;
    int ack;

    for (bit = 7; bit >= 0; bit--) {
        scl_drive_low();

        if (byte & (1u << bit))
            sda_release();
        else
            sda_drive_low();

        sccb_delay();
        scl_release();
        sccb_delay();
    }

    scl_drive_low();
    sda_release();
    sccb_delay();

    scl_release();
    sccb_delay();
    ack = (gpio_read(PIN_SIOD) == 0);

    scl_drive_low();
    sccb_delay();

    return ack;
}

uint8_t sccb_read_byte(int send_ack)
{
    uint8_t value = 0;
    int bit;

    sda_release();

    for (bit = 7; bit >= 0; bit--) {
        scl_drive_low();
        sccb_delay();

        scl_release();
        sccb_delay();

        if (gpio_read(PIN_SIOD))
            value |= (uint8_t)(1u << bit);
    }

    scl_drive_low();
    if (send_ack)
        sda_drive_low();
    else
        sda_release();

    sccb_delay();
    scl_release();
    sccb_delay();
    scl_drive_low();
    sda_release();
    sccb_delay();

    return value;
}

static int write_stage(uint8_t byte, const char *stage)
{
    int ack = sccb_write_byte(byte);

    printf("%s: wrote 0x%02X, %s\n", stage, byte, ack ? "ACK received" : "NO ACK");
    return ack;
}

int ov7670_write_reg(uint8_t reg, uint8_t value)
{
    int ok = 1;

    printf("SCCB write reg 0x%02X <- 0x%02X\n", reg, value);
    sccb_start();
    ok &= write_stage(OV7670_ADDR_WR, "  slave write address 0x42");
    ok &= write_stage(reg, "  register index");
    ok &= write_stage(value, "  register value");
    sccb_stop();

    if (!ok)
        printf("SCCB write reg 0x%02X failed due to missing ACK\n", reg);

    return ok;
}

int ov7670_read_reg(uint8_t reg, uint8_t *value)
{
    int ok = 1;

    printf("SCCB read reg 0x%02X\n", reg);

    sccb_start();
    ok &= write_stage(OV7670_ADDR_WR, "  phase 1 slave write address 0x42");
    ok &= write_stage(reg, "  phase 1 register index");
    sccb_stop();

    if (!ok) {
        printf("SCCB read reg 0x%02X failed during register select phase\n", reg);
        return 0;
    }

    sccb_start();
    ok &= write_stage(OV7670_ADDR_RD, "  phase 2 slave read address 0x43");

    if (!ok) {
        sccb_stop();
        printf("SCCB read reg 0x%02X failed during read address phase\n", reg);
        return 0;
    }

    *value = sccb_read_byte(0);      /* NACK after single-byte read */
    printf("  data byte: read 0x%02X, master sent NACK\n", *value);
    sccb_stop();

    return 1;
}

static void *xclk_thread_main(void *arg)
{
    (void)arg;

    gpio_write(PIN_XCLK, 0);
    gpio_set_output(PIN_XCLK);

    while (xclk_running) {
        gpio_write(PIN_XCLK, 1);
        usleep(XCLK_HALF_PERIOD_US);
        gpio_write(PIN_XCLK, 0);
        usleep(XCLK_HALF_PERIOD_US);
    }

    gpio_write(PIN_XCLK, 0);
    return NULL;
}

static int map_gpio1(void)
{
    void *map;

    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        fprintf(stderr, "open /dev/mem failed: %s\n", strerror(errno));
        return 0;
    }

    map = mmap(NULL, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
               mem_fd, GPIO1_BASE_PHYS);
    if (map == MAP_FAILED) {
        fprintf(stderr, "mmap GPIO1 at 0x%lX failed: %s\n",
                GPIO1_BASE_PHYS, strerror(errno));
        close(mem_fd);
        mem_fd = -1;
        return 0;
    }

    gpio1 = (volatile uint32_t *)map;
    return 1;
}

static void unmap_gpio1(void)
{
    if (gpio1 != NULL && gpio1 != MAP_FAILED) {
        munmap((void *)gpio1, GPIO_MAP_SIZE);
        gpio1 = NULL;
    }

    if (mem_fd >= 0) {
        close(mem_fd);
        mem_fd = -1;
    }
}

static int start_xclk(void)
{
    xclk_running = 1;
    if (pthread_create(&xclk_thread, NULL, xclk_thread_main, NULL) != 0) {
        fprintf(stderr, "pthread_create for XCLK failed\n");
        xclk_running = 0;
        return 0;
    }

    return 1;
}

static void stop_xclk(void)
{
    if (xclk_running) {
        xclk_running = 0;
        pthread_join(xclk_thread, NULL);
    }
}

static void init_gpio_lines(void)
{
    /*
     * Put SCL/SDA output latches low before any direction changes. Releasing
     * them means input/high-Z, not output-high.
     */
    gpio_write(PIN_SIOC, 0);
    gpio_write(PIN_SIOD, 0);
    gpio_set_input(PIN_SIOC);
    gpio_set_input(PIN_SIOD);

    gpio_write(PIN_PWDN, 1);     /* keep sensor powered down until XCLK runs */
    gpio_set_output(PIN_PWDN);

    gpio_write(PIN_RST, 1);
    gpio_set_output(PIN_RST);

    gpio_write(PIN_XCLK, 0);
    gpio_set_output(PIN_XCLK);
}

static void ov7670_power_reset_sequence(void)
{
    printf("Waiting %.0f ms with slow XCLK running\n", STARTUP_XCLK_US / 1000.0);
    usleep(STARTUP_XCLK_US);

    printf("Driving PWDN low to enable OV7670\n");
    gpio_write(PIN_PWDN, 0);

    printf("Pulsing RST low for %.0f ms\n", RESET_LOW_US / 1000.0);
    gpio_write(PIN_RST, 0);
    usleep(RESET_LOW_US);

    printf("Driving RST high\n");
    gpio_write(PIN_RST, 1);

    printf("Waiting %.0f ms after reset\n", AFTER_RESET_US / 1000.0);
    usleep(AFTER_RESET_US);
}

int main(void)
{
    uint8_t pid = 0;
    uint8_t ver = 0;
    int pid_ok;
    int ver_ok;
    int exit_code = EXIT_FAILURE;

    printf("OV7670 SCCB probe using GPIO1 pins\n");
    printf("GPIO1 base 0x%lX, OV7670 7-bit address 0x%02X\n",
           GPIO1_BASE_PHYS, OV7670_ADDR_7BIT);

    if (!map_gpio1())
        return EXIT_FAILURE;

    init_gpio_lines();

    if (!start_xclk())
        goto out;

    printf("Slow test XCLK running on GPIO1[%d], about %u Hz\n",
           PIN_XCLK, 1000000u / (2u * XCLK_HALF_PERIOD_US));

    ov7670_power_reset_sequence();

    pid_ok = ov7670_read_reg(OV7670_REG_PID, &pid);
    ver_ok = ov7670_read_reg(OV7670_REG_VER, &ver);

    printf("\nProbe results:\n");
    if (pid_ok)
        printf("  PID register 0x0A = 0x%02X%s\n",
               pid, pid == 0x76 ? " (expected OV7670 PID)" : "");
    else
        printf("  PID register 0x0A read failed\n");

    if (ver_ok)
        printf("  VER register 0x0B = 0x%02X\n", ver);
    else
        printf("  VER register 0x0B read failed\n");

    if (pid_ok && ver_ok)
        exit_code = EXIT_SUCCESS;

out:
    stop_xclk();

    /*
     * Leave SCCB released and slow XCLK low. Keep PWDN low and RST high so the
     * sensor remains enabled after a successful probe.
     */
    gpio_set_input(PIN_SIOC);
    gpio_set_input(PIN_SIOD);
    gpio_write(PIN_XCLK, 0);
    gpio_set_output(PIN_XCLK);

    unmap_gpio1();
    return exit_code;
}
