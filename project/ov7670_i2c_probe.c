#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BRIDGE_BASE 0xFF200000u
#define LW_BRIDGE_SPAN 0x1000u

#define GPIO1_I2C_OFFSET 0x40u
#define PIO_DATA 0x00u
#define PIO_DIR 0x04u

#define SCL_BIT (1u << 0) /* GPIO_1[22] */
#define SDA_BIT (1u << 1) /* GPIO_1[23] */
#define I2C_BITS (SCL_BIT | SDA_BIT)

#define OV7670_ADDR 0x21u
#define REG_PID 0x0au
#define REG_VER 0x0bu

#define I2C_DELAY_US 5
#define SCCB_STOP_US 1000

static volatile uint32_t *pio_data;
static volatile uint32_t *pio_dir;

static void delay_i2c(void)
{
    usleep(I2C_DELAY_US);
}

static void drive_low(uint32_t bit)
{
    *pio_data &= ~bit;
    *pio_dir |= bit;
    delay_i2c();
}

static void release_line(uint32_t bit)
{
    *pio_data &= ~bit;
    *pio_dir &= ~bit;
    delay_i2c();
}

static int read_line(uint32_t bit)
{
    return (*pio_data & bit) != 0;
}

static int wait_high(uint32_t bit)
{
    for (unsigned i = 0; i < 1000; i++) {
        if (read_line(bit)) {
            return 0;
        }
        usleep(1);
    }
    return -1;
}

static int scl_high(void)
{
    release_line(SCL_BIT);
    return wait_high(SCL_BIT);
}

static void scl_low(void)
{
    drive_low(SCL_BIT);
}

static void sda_high(void)
{
    release_line(SDA_BIT);
}

static void sda_low(void)
{
    drive_low(SDA_BIT);
}

static int i2c_start(void)
{
    sda_high();
    if (scl_high() < 0) {
        return -1;
    }
    sda_low();
    scl_low();
    return 0;
}

static int i2c_stop(void)
{
    sda_low();
    if (scl_high() < 0) {
        return -1;
    }
    sda_high();
    return 0;
}

static void print_bus_state(const char *label)
{
    uint32_t data = *pio_data;
    uint32_t dir = *pio_dir;

    printf("%s: DATA=0x%08x DIR=0x%08x SCL=%d SDA=%d\n", label, data, dir,
           (data & SCL_BIT) != 0, (data & SDA_BIT) != 0);
}

static void recover_bus(void)
{
    sda_high();
    for (unsigned i = 0; i < 9; i++) {
        if (scl_high() < 0) {
            break;
        }
        scl_low();
    }
    i2c_stop();
    usleep(SCCB_STOP_US);
}

static int i2c_write_byte(uint8_t value)
{
    for (int bit = 7; bit >= 0; bit--) {
        if (value & (1u << bit)) {
            sda_high();
        } else {
            sda_low();
        }

        if (scl_high() < 0) {
            return -1;
        }
        scl_low();
    }

    sda_high();
    if (scl_high() < 0) {
        return -1;
    }

    int ack = !read_line(SDA_BIT);
    scl_low();
    return ack ? 0 : -2;
}

static int i2c_read_byte(uint8_t *value, int nack)
{
    uint8_t byte = 0;

    sda_high();
    for (int bit = 7; bit >= 0; bit--) {
        if (scl_high() < 0) {
            return -1;
        }
        if (read_line(SDA_BIT)) {
            byte |= (uint8_t)(1u << bit);
        }
        scl_low();
    }

    if (nack) {
        sda_high();
    } else {
        sda_low();
    }

    if (scl_high() < 0) {
        return -1;
    }
    scl_low();
    sda_high();

    *value = byte;
    return 0;
}

static int ov7670_read_reg(uint8_t reg, uint8_t *value)
{
    int ret;

    ret = i2c_start();
    if (ret < 0) {
        return ret;
    }
    ret = i2c_write_byte((uint8_t)(OV7670_ADDR << 1));
    if (ret < 0) {
        i2c_stop();
        return ret;
    }
    ret = i2c_write_byte(reg);
    if (ret < 0) {
        i2c_stop();
        return ret;
    }
    i2c_stop();
    usleep(SCCB_STOP_US);

    ret = i2c_start();
    if (ret < 0) {
        return ret;
    }
    ret = i2c_write_byte((uint8_t)((OV7670_ADDR << 1) | 1u));
    if (ret < 0) {
        i2c_stop();
        return ret;
    }
    ret = i2c_read_byte(value, 1);
    i2c_stop();
    usleep(SCCB_STOP_US);
    return ret;
}

int main(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    void *map = mmap(NULL, LW_BRIDGE_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd, LW_BRIDGE_BASE);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    pio_data = (volatile uint32_t *)((uint8_t *)map + GPIO1_I2C_OFFSET + PIO_DATA);
    pio_dir = (volatile uint32_t *)((uint8_t *)map + GPIO1_I2C_OFFSET + PIO_DIR);

    uint32_t old_data = *pio_data;
    uint32_t old_dir = *pio_dir;

    *pio_data = old_data & ~I2C_BITS;
    *pio_dir = old_dir & ~I2C_BITS;
    usleep(SCCB_STOP_US);

    if (!read_line(SCL_BIT) || !read_line(SDA_BIT)) {
        print_bus_state("released before recovery");
        recover_bus();
    }

    if (!read_line(SCL_BIT) || !read_line(SDA_BIT)) {
        print_bus_state("released after recovery");
        fprintf(stderr, "I2C lines still not high after release/recovery.\n");
        fprintf(stderr, "Open-drain mode needs pull-ups: SCL=GPIO_1[22], SDA=GPIO_1[23].\n");
        *pio_data = old_data;
        *pio_dir = old_dir;
        munmap(map, LW_BRIDGE_SPAN);
        close(fd);
        return 1;
    }

    uint8_t pid = 0;
    uint8_t ver = 0;
    int ret_pid = ov7670_read_reg(REG_PID, &pid);
    int ret_ver = ov7670_read_reg(REG_VER, &ver);

    i2c_stop();
    *pio_data = old_data;
    *pio_dir = old_dir;
    munmap(map, LW_BRIDGE_SPAN);
    close(fd);

    if (ret_pid < 0 || ret_ver < 0) {
        fprintf(stderr, "OV7670 read failed: PID ret=%d, VER ret=%d\n",
                ret_pid, ret_ver);
        fprintf(stderr, "ret=-1 means SCL stuck low, ret=-2 means no ACK\n");
        return 1;
    }

    printf("OV7670 PID=0x%02x VER=0x%02x\n", pid, ver);
    return 0;
}
