#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BRIDGE_BASE 0xFF200000u
#define LW_BRIDGE_SPAN 0x1000u

#define GPIO0_I2C_OFFSET 0x120u
#define GPIO1_I2C_OFFSET 0x160u
#define PIO_DATA 0x00u
#define PIO_DIR 0x04u

#define SCL_BIT (1u << 0)
#define SDA_BIT (1u << 1)
#define I2C_BITS (SCL_BIT | SDA_BIT)

#define OV7670_ADDR 0x21u
#define REG_GAIN 0x00u
#define REG_VREF 0x03u
#define REG_COM1 0x04u
#define REG_AECHH 0x07u
#define REG_PID 0x0au
#define REG_VER 0x0bu
#define REG_COM3 0x0cu
#define REG_AECH 0x10u
#define REG_CLKRC 0x11u
#define REG_COM7 0x12u
#define REG_COM8 0x13u
#define REG_COM9 0x14u
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
#define REG_RGB444 0x8cu

#define COM7_RESET 0x80u
#define COM7_YUV 0x00u
#define COM8_FASTAEC 0x80u
#define COM8_AECSTEP 0x40u
#define COM8_AGC 0x04u
#define COM8_AEC 0x01u
#define COM15_FULL_RANGE 0xc0u
#define TSLB_FIXED_UV 0x10u

#define I2C_DELAY_US 5
#define SCCB_STOP_US 1000
#define DEFAULT_GAIN 0x20u
#define DEFAULT_AGC 0
#define DEFAULT_AEC 1
#define VGA_VREF_LOW_BITS 0x0au
#define MAX_GAIN 0x3ffu
#define MAX_EXPOSURE 0xffffu
#define MAX_REG8 0xffu

#define CAMERA_A_SELECT (1u << 0)
#define CAMERA_B_SELECT (1u << 1)
#define CAMERA_BOTH_SELECT (CAMERA_A_SELECT | CAMERA_B_SELECT)

struct camera_reg {
    const char *name;
    uint8_t reg;
    uint8_t val;
    uint8_t mask;
};

struct camera_settings {
    unsigned gain;
    unsigned exposure;
    unsigned gain_ceiling;
    int agc;
    int aec;
    int exposure_set;
    int gain_ceiling_set;
};

static const struct camera_reg grayscale_vga_regs[] = {
    {"CLKRC", REG_CLKRC, 0x40, 0x7f},
    {"COM7", REG_COM7, COM7_YUV, 0xff},
    {"COM3", REG_COM3, 0x00, 0xff},
    {"COM14", REG_COM14, 0x00, 0xff},
    {"HSTART", REG_HSTART, 0x13, 0xff},
    {"HSTOP", REG_HSTOP, 0x01, 0xff},
    {"HREF", REG_HREF, 0xb6, 0xff},
    {"VSTART", REG_VSTART, 0x02, 0xff},
    {"VSTOP", REG_VSTOP, 0x7a, 0xff},
    {"TSLB", REG_TSLB, TSLB_FIXED_UV, 0xff},
    {"MANU", REG_MANU, 0x80, 0xff},
    {"MANV", REG_MANV, 0x80, 0xff},
    {"COM13", REG_COM13, 0x00, 0xff},
    {"RGB444", REG_RGB444, 0x00, 0xff},
    {"COM15", REG_COM15, COM15_FULL_RANGE, 0xff},
    {"COM10", REG_COM10, 0x00, 0xff},
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
    return ov7670_write_reg(REG_VREF, (uint8_t)(VGA_VREF_LOW_BITS | high));
}

static uint8_t ov7670_com8_value(const struct camera_settings *settings)
{
    uint8_t value = COM8_FASTAEC | COM8_AECSTEP;

    if (settings->agc) {
        value |= COM8_AGC;
    }
    if (settings->aec) {
        value |= COM8_AEC;
    }

    return value;
}

static int ov7670_set_exposure(unsigned exposure)
{
    uint8_t com1 = 0;
    uint8_t aechh = 0;
    int ret;

    ret = ov7670_read_reg(REG_COM1, &com1);
    if (ret < 0) {
        return ret;
    }
    ret = ov7670_read_reg(REG_AECHH, &aechh);
    if (ret < 0) {
        return ret;
    }

    ret = ov7670_write_reg(REG_COM1,
                           (uint8_t)((com1 & 0xfcu) | (exposure & 0x03u)));
    if (ret < 0) {
        return ret;
    }
    ret = ov7670_write_reg(REG_AECH, (uint8_t)((exposure >> 2) & 0xffu));
    if (ret < 0) {
        return ret;
    }
    return ov7670_write_reg(REG_AECHH,
                            (uint8_t)((aechh & 0xc0u) |
                                      ((exposure >> 10) & 0x3fu)));
}

static int ov7670_configure_grayscale_vga(const struct camera_settings *settings)
{
    int ret;

    ret = ov7670_write_reg(REG_COM7, COM7_RESET);
    if (ret < 0) {
        return ret;
    }
    usleep(10000);

    ret = ov7670_write_table(grayscale_vga_regs,
                             sizeof(grayscale_vga_regs) /
                                 sizeof(grayscale_vga_regs[0]));
    if (ret < 0) {
        return ret;
    }
    ret = ov7670_set_gain(settings->gain);
    if (ret < 0) {
        return ret;
    }
    if (settings->exposure_set) {
        ret = ov7670_set_exposure(settings->exposure);
        if (ret < 0) {
            return ret;
        }
    }
    if (settings->gain_ceiling_set) {
        ret = ov7670_write_reg(REG_COM9, (uint8_t)settings->gain_ceiling);
        if (ret < 0) {
            return ret;
        }
    }
    return ov7670_write_reg(REG_COM8, ov7670_com8_value(settings));
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

static int ov7670_verify_exposure(unsigned expected)
{
    uint8_t com1 = 0;
    uint8_t aech = 0;
    uint8_t aechh = 0;
    unsigned actual;
    int ret;
    int pass;

    ret = ov7670_read_reg(REG_COM1, &com1);
    if (ret < 0) {
        printf("%-18s read failed ret=%d FAIL\n", "EXPOSURE", ret);
        return -1;
    }
    ret = ov7670_read_reg(REG_AECH, &aech);
    if (ret < 0) {
        printf("%-18s read failed ret=%d FAIL\n", "EXPOSURE", ret);
        return -1;
    }
    ret = ov7670_read_reg(REG_AECHH, &aechh);
    if (ret < 0) {
        printf("%-18s read failed ret=%d FAIL\n", "EXPOSURE", ret);
        return -1;
    }

    actual = (unsigned)(com1 & 0x03u) |
             ((unsigned)aech << 2) |
             ((unsigned)(aechh & 0x3fu) << 10);
    pass = actual == expected;

    printf("%-18s read=0x%04x expect=0x%04x %s\n",
           "EXPOSURE", actual, expected, pass ? "OK" : "FAIL");
    return pass ? 0 : -1;
}

static int ov7670_verify_grayscale_vga(const struct camera_settings *settings)
{
    unsigned failures = 0;
    uint8_t gain_low = (uint8_t)(settings->gain & 0xffu);
    uint8_t gain_high = (uint8_t)(((settings->gain >> 8) & 0x03u) << 6);
    uint8_t vref = (uint8_t)(VGA_VREF_LOW_BITS | gain_high);
    uint8_t com8 = ov7670_com8_value(settings);

    printf("Verifying OV7670 register readback before capture:\n");

    for (size_t i = 0; i < sizeof(grayscale_vga_regs) /
                               sizeof(grayscale_vga_regs[0]);
         i++) {
        const struct camera_reg *v = &grayscale_vga_regs[i];
        if (ov7670_verify_reg(v->name, v->reg, v->val, v->mask) < 0) {
            failures++;
        }
    }

    if (ov7670_verify_reg("COM8", REG_COM8, com8, 0xff) < 0) {
        failures++;
    }

    if (settings->gain_ceiling_set &&
        ov7670_verify_reg("COM9", REG_COM9, (uint8_t)settings->gain_ceiling,
                          0xff) < 0) {
        failures++;
    }

    if (settings->agc) {
        printf("%-18s auto gain enabled; exact manual gain readback skipped\n",
               "GAIN");
        if (ov7670_verify_reg("VREF", REG_VREF, VGA_VREF_LOW_BITS, 0x0f) < 0) {
            failures++;
        }
    } else {
        if (ov7670_verify_reg("GAIN", REG_GAIN, gain_low, 0xff) < 0) {
            failures++;
        }
        if (ov7670_verify_reg("VREF", REG_VREF, vref, 0xff) < 0) {
            failures++;
        }
    }

    if (settings->exposure_set) {
        if (settings->aec) {
            printf("%-18s auto exposure enabled; exact readback skipped\n",
                   "EXPOSURE");
        } else if (ov7670_verify_exposure(settings->exposure) < 0) {
            failures++;
        }
    }

    if (failures) {
        fprintf(stderr, "%u register verification failure(s); capture not ready\n",
                failures);
        return -1;
    }

    printf("Register verification passed; capture may start.\n");
    return 0;
}

static int parse_unsigned(const char *text, unsigned max, unsigned *value_out)
{
    char *end = NULL;
    unsigned long value;

    errno = 0;
    value = strtoul(text, &end, 0);
    if (errno || *end || value > max) {
        return -1;
    }
    *value_out = (unsigned)value;
    return 0;
}

static int parse_bool(const char *text, int *value_out)
{
    if (strcmp(text, "1") == 0 || strcmp(text, "on") == 0 ||
        strcmp(text, "ON") == 0 || strcmp(text, "true") == 0 ||
        strcmp(text, "TRUE") == 0 || strcmp(text, "yes") == 0 ||
        strcmp(text, "YES") == 0) {
        *value_out = 1;
        return 0;
    }
    if (strcmp(text, "0") == 0 || strcmp(text, "off") == 0 ||
        strcmp(text, "OFF") == 0 || strcmp(text, "false") == 0 ||
        strcmp(text, "FALSE") == 0 || strcmp(text, "no") == 0 ||
        strcmp(text, "NO") == 0) {
        *value_out = 0;
        return 0;
    }
    return -1;
}

static int parse_gain(const char *text, unsigned *gain)
{
    return parse_unsigned(text, MAX_GAIN, gain);
}

static int parse_camera_select(const char *text, unsigned *cameras)
{
    if (strcmp(text, "A") == 0 || strcmp(text, "a") == 0 ||
        strcmp(text, "0") == 0) {
        *cameras = CAMERA_A_SELECT;
        return 0;
    }
    if (strcmp(text, "B") == 0 || strcmp(text, "b") == 0 ||
        strcmp(text, "1") == 0) {
        *cameras = CAMERA_B_SELECT;
        return 0;
    }
    if (strcmp(text, "both") == 0 || strcmp(text, "BOTH") == 0 ||
        strcmp(text, "all") == 0 || strcmp(text, "ALL") == 0) {
        *cameras = CAMERA_BOTH_SELECT;
        return 0;
    }
    return -1;
}

static int parse_setting(const char *text, struct camera_settings *settings)
{
    const char *value = strchr(text, '=');
    size_t name_len;

    if (!value) {
        return -1;
    }
    name_len = (size_t)(value - text);
    value++;

    if (name_len == 4 && strncmp(text, "gain", name_len) == 0) {
        return parse_unsigned(value, MAX_GAIN, &settings->gain);
    }
    if (name_len == 3 && strncmp(text, "agc", name_len) == 0) {
        return parse_bool(value, &settings->agc);
    }
    if (name_len == 3 && strncmp(text, "aec", name_len) == 0) {
        return parse_bool(value, &settings->aec);
    }
    if (name_len == 8 && strncmp(text, "exposure", name_len) == 0) {
        if (parse_unsigned(value, MAX_EXPOSURE, &settings->exposure) < 0) {
            return -1;
        }
        settings->exposure_set = 1;
        return 0;
    }
    if (name_len == 7 && strncmp(text, "ceiling", name_len) == 0) {
        if (parse_unsigned(value, MAX_REG8, &settings->gain_ceiling) < 0) {
            return -1;
        }
        settings->gain_ceiling_set = 1;
        return 0;
    }
    if (name_len == 4 && strncmp(text, "com9", name_len) == 0) {
        if (parse_unsigned(value, MAX_REG8, &settings->gain_ceiling) < 0) {
            return -1;
        }
        settings->gain_ceiling_set = 1;
        return 0;
    }

    return -1;
}

static void usage(const char *name)
{
    fprintf(stderr, "usage: %s [A|B|both] [gain] [key=value...]\n", name);
    fprintf(stderr, "  gain=0..0x3ff       manual sensor gain (default 0x%02x)\n",
            DEFAULT_GAIN);
    fprintf(stderr, "  agc=0|1             auto gain control (default %d)\n",
            DEFAULT_AGC);
    fprintf(stderr, "  aec=0|1             auto exposure control (default %d)\n",
            DEFAULT_AEC);
    fprintf(stderr, "  exposure=0..0xffff  manual exposure; use with aec=0\n");
    fprintf(stderr, "  ceiling=0..0xff     COM9 auto-gain ceiling; use with agc=1\n");
}

static int configure_camera(void *map, uint32_t i2c_offset, const char *label,
                            const struct camera_settings *settings)
{
    uint32_t old_data;
    uint32_t old_dir;
    uint8_t pid = 0;
    uint8_t ver = 0;
    int ret_pid;
    int ret_ver;
    int ret_cfg = 0;
    int ret_verify = 0;
    int failed = 0;

    printf("\nConfiguring %s\n", label);

    pio_data = (volatile uint32_t *)((uint8_t *)map + i2c_offset + PIO_DATA);
    pio_dir = (volatile uint32_t *)((uint8_t *)map + i2c_offset + PIO_DIR);

    old_data = *pio_data;
    old_dir = *pio_dir;

    *pio_data = old_data & ~I2C_BITS;
    *pio_dir = old_dir & ~I2C_BITS;
    usleep(SCCB_STOP_US);

    if (!read_line(SCL_BIT) || !read_line(SDA_BIT)) {
        fprintf(stderr, "%s: SCL/SDA did not release high; check pull-ups and wiring\n",
                label);
        failed = 1;
        goto out;
    }

    ret_pid = ov7670_read_reg(REG_PID, &pid);
    ret_ver = ov7670_read_reg(REG_VER, &ver);

    if (ret_pid == 0 && ret_ver == 0) {
        ret_cfg = ov7670_configure_grayscale_vga(settings);
        if (ret_cfg == 0) {
            ret_verify = ov7670_verify_grayscale_vga(settings);
        }
    }

    if (ret_pid < 0 || ret_ver < 0) {
        fprintf(stderr, "%s: OV7670 read failed: PID ret=%d, VER ret=%d\n",
                label, ret_pid, ret_ver);
        fprintf(stderr, "ret=-1 means SCL stuck low, ret=-2 means no ACK\n");
        failed = 1;
        goto out;
    }

    if (ret_cfg < 0) {
        fprintf(stderr, "%s: OV7670 grayscale VGA configuration failed: ret=%d\n",
                label, ret_cfg);
        fprintf(stderr, "ret=-1 means SCL stuck low, ret=-2 means no ACK\n");
        failed = 1;
        goto out;
    }

    if (ret_verify < 0) {
        failed = 1;
        goto out;
    }

    printf("%s: OV7670 PID=0x%02x VER=0x%02x configured grayscale VGA\n",
           label, pid, ver);
    printf("%s: gain=0x%03x agc=%d aec=%d",
           label, settings->gain, settings->agc, settings->aec);
    if (settings->exposure_set) {
        printf(" exposure=0x%04x", settings->exposure);
    }
    if (settings->gain_ceiling_set) {
        printf(" ceiling=0x%02x", settings->gain_ceiling);
    }
    printf("\n");

out:
    i2c_stop();
    *pio_data = old_data;
    *pio_dir = old_dir;
    return failed ? -1 : 0;
}

int main(int argc, char **argv)
{
    unsigned cameras = CAMERA_A_SELECT;
    struct camera_settings settings = {
        .gain = DEFAULT_GAIN,
        .exposure = 0,
        .gain_ceiling = 0,
        .agc = DEFAULT_AGC,
        .aec = DEFAULT_AEC,
        .exposure_set = 0,
        .gain_ceiling_set = 0,
    };
    int failed = 0;
    int argi = 1;

    if (argc > 8) {
        usage(argv[0]);
        return 2;
    }

    if (argi < argc && parse_camera_select(argv[argi], &cameras) == 0) {
        argi++;
    }

    if (argi < argc && strchr(argv[argi], '=') == NULL) {
        if (parse_gain(argv[argi], &settings.gain) < 0) {
            usage(argv[0]);
            return 2;
        }
        argi++;
    }

    while (argi < argc) {
        if (parse_setting(argv[argi], &settings) < 0) {
            usage(argv[0]);
            return 2;
        }
        argi++;
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

    if ((cameras & CAMERA_A_SELECT) &&
        configure_camera(map, GPIO0_I2C_OFFSET, "camera A (GPIO0)",
                         &settings) < 0) {
        failed = 1;
    }
    if ((cameras & CAMERA_B_SELECT) &&
        configure_camera(map, GPIO1_I2C_OFFSET, "camera B (GPIO1)",
                         &settings) < 0) {
        failed = 1;
    }

    munmap(map, LW_BRIDGE_SPAN);
    close(fd);

    return failed ? 1 : 0;
}
