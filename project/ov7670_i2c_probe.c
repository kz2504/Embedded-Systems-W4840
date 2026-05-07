#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
#define REG_GAIN 0x00u
#define REG_VREF 0x03u
#define REG_PID 0x0au
#define REG_VER 0x0bu
#define REG_COM3 0x0cu
#define REG_CLKRC 0x11u
#define REG_COM7 0x12u
#define REG_COM8 0x13u
#define REG_COM10 0x15u
#define REG_HSTART 0x17u
#define REG_HSTOP 0x18u
#define REG_VSTART 0x19u
#define REG_VSTOP 0x1au
#define REG_HREF 0x32u
#define REG_TSLB 0x3au
#define REG_COM13 0x3du
#define REG_COM14 0x3eu
#define REG_COM15 0x40u
#define REG_MANU 0x67u
#define REG_MANV 0x68u
#define REG_SCALING_XSC 0x70u
#define REG_SCALING_YSC 0x71u
#define REG_SCALING_DCWCTR 0x72u
#define REG_SCALING_PCLK_DIV 0x73u
#define REG_RGB444 0x8cu

#define COM7_RESET 0x80u
#define COM7_QVGA 0x10u
#define COM7_YUV 0x00u
#define COM8_FASTAEC 0x80u
#define COM8_AECSTEP 0x40u
#define COM8_AEC 0x01u
#define COM14_DCWEN 0x10u
#define COM15_FULL_RANGE 0xc0u
#define TSLB_FIXED_UV 0x10u

#define I2C_DELAY_US 5
#define SCCB_STOP_US 1000
#define DEFAULT_GAIN 0x20u
#define QVGA_VREF_LOW_BITS 0x0au

struct camera_reg {
    const char *name;
    uint8_t reg;
    uint8_t val;
    uint8_t mask;
};

static const struct camera_reg grayscale_qvga_regs[] = {
    {"CLKRC", REG_CLKRC, 0x01, 0xff},
    {"COM7", REG_COM7, COM7_QVGA | COM7_YUV, 0xff},
    {"COM3", REG_COM3, 0x04, 0xff},
    {"COM14", REG_COM14, COM14_DCWEN | 0x09, 0xff},
    {"SCALING_XSC", REG_SCALING_XSC, 0x3a, 0xff},
    {"SCALING_YSC", REG_SCALING_YSC, 0x35, 0xff},
    {"SCALING_DCWCTR", REG_SCALING_DCWCTR, 0x11, 0xff},
    {"SCALING_PCLK_DIV", REG_SCALING_PCLK_DIV, 0xf1, 0xff},
    {"HSTART", REG_HSTART, 0x16, 0xff},
    {"HSTOP", REG_HSTOP, 0x04, 0xff},
    {"HREF", REG_HREF, 0xa4, 0xff},
    {"VSTART", REG_VSTART, 0x02, 0xff},
    {"VSTOP", REG_VSTOP, 0x7a, 0xff},
    {"TSLB", REG_TSLB, TSLB_FIXED_UV, 0xff},
    {"MANU", REG_MANU, 0x80, 0xff},
    {"MANV", REG_MANV, 0x80, 0xff},
    {"COM13", REG_COM13, 0x00, 0xff},
    {"RGB444", REG_RGB444, 0x00, 0xff},
    {"COM15", REG_COM15, COM15_FULL_RANGE, 0xff},
    {"COM10", REG_COM10, 0x00, 0xff},
    {"COM8", REG_COM8, COM8_FASTAEC | COM8_AECSTEP | COM8_AEC, 0xff},
};

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

static int ov7670_write_reg(uint8_t reg, uint8_t value)
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
    ret = i2c_write_byte(value);
    i2c_stop();
    usleep(SCCB_STOP_US);
    return ret;
}

static int ov7670_write_table(const struct camera_reg *regs, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        int ret = ov7670_write_reg(regs[i].reg, regs[i].val);
        if (ret < 0) {
            fprintf(stderr, "write %-18s reg 0x%02x failed: ret=%d\n",
                    regs[i].name, regs[i].reg, ret);
            return ret;
        }
    }
    return 0;
}

static int ov7670_set_gain(unsigned gain)
{
    uint8_t low = (uint8_t)(gain & 0xffu);
    uint8_t high = (uint8_t)(((gain >> 8) & 0x03u) << 6);
    int ret;

    ret = ov7670_write_reg(REG_GAIN, low);
    if (ret < 0) {
        return ret;
    }
    return ov7670_write_reg(REG_VREF, (uint8_t)(QVGA_VREF_LOW_BITS | high));
}

static int ov7670_configure_grayscale_qvga(unsigned gain)
{
    int ret;

    ret = ov7670_write_reg(REG_COM7, COM7_RESET);
    if (ret < 0) {
        return ret;
    }
    usleep(10000);

    ret = ov7670_write_table(grayscale_qvga_regs,
                             sizeof(grayscale_qvga_regs) /
                                 sizeof(grayscale_qvga_regs[0]));
    if (ret < 0) {
        return ret;
    }
    return ov7670_set_gain(gain);
}

static int ov7670_verify_reg(const char *name, uint8_t reg, uint8_t expected,
                             uint8_t mask)
{
    uint8_t actual = 0;
    int ret = ov7670_read_reg(reg, &actual);
    int pass;

    if (ret < 0) {
        printf("%-18s reg=0x%02x read failed ret=%d FAIL\n", name, reg, ret);
        return -1;
    }

    pass = (actual & mask) == (expected & mask);
    printf("%-18s reg=0x%02x read=0x%02x expect=0x%02x mask=0x%02x %s\n",
           name, reg, actual, expected, mask, pass ? "OK" : "FAIL");
    return pass ? 0 : -1;
}

static int ov7670_verify_grayscale_qvga(unsigned gain)
{
    unsigned failures = 0;
    uint8_t gain_low = (uint8_t)(gain & 0xffu);
    uint8_t gain_high = (uint8_t)(((gain >> 8) & 0x03u) << 6);
    uint8_t vref = (uint8_t)(QVGA_VREF_LOW_BITS | gain_high);

    printf("Verifying OV7670 register readback before capture:\n");

    for (size_t i = 0; i < sizeof(grayscale_qvga_regs) /
                               sizeof(grayscale_qvga_regs[0]);
         i++) {
        const struct camera_reg *v = &grayscale_qvga_regs[i];
        if (ov7670_verify_reg(v->name, v->reg, v->val, v->mask) < 0) {
            failures++;
        }
    }

    if (ov7670_verify_reg("GAIN", REG_GAIN, gain_low, 0xff) < 0) {
        failures++;
    }
    if (ov7670_verify_reg("VREF", REG_VREF, vref, 0xff) < 0) {
        failures++;
    }

    if (failures) {
        fprintf(stderr, "%u register verification failure(s); capture not ready\n",
                failures);
        return -1;
    }

    printf("Register verification passed; capture may start.\n");
    return 0;
}

static int parse_gain(const char *text, unsigned *gain)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || *end || value > 0x3fful) {
        return -1;
    }
    *gain = (unsigned)value;
    return 0;
}

static void usage(const char *name)
{
    fprintf(stderr, "usage: %s [gain]\n", name);
    fprintf(stderr, "  gain: 10-bit manual gain value, 0..0x3ff (default 0x%02x)\n",
            DEFAULT_GAIN);
}

int main(int argc, char **argv)
{
    unsigned gain = DEFAULT_GAIN;

    if (argc > 2) {
        usage(argv[0]);
        return 2;
    }
    if (argc == 2 && parse_gain(argv[1], &gain) < 0) {
        usage(argv[0]);
        return 2;
    }

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
        fprintf(stderr, "SCL/SDA did not release high; check pull-ups and wiring\n");
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
    int ret_cfg = 0;
    int ret_verify = 0;

    if (ret_pid == 0 && ret_ver == 0) {
        ret_cfg = ov7670_configure_grayscale_qvga(gain);
        if (ret_cfg == 0) {
            ret_verify = ov7670_verify_grayscale_qvga(gain);
        }
    }

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

    if (ret_cfg < 0) {
        fprintf(stderr, "OV7670 grayscale QVGA configuration failed: ret=%d\n",
                ret_cfg);
        fprintf(stderr, "ret=-1 means SCL stuck low, ret=-2 means no ACK\n");
        return 1;
    }

    if (ret_verify < 0) {
        return 1;
    }

    printf("OV7670 PID=0x%02x VER=0x%02x configured grayscale QVGA, gain=0x%03x\n",
           pid, ver, gain);
    return 0;
}
