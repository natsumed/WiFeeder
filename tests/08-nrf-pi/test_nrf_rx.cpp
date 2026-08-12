/*
 * Pi-side NRF24L01+ RX smoke test via Linux spidev + GPIO CE.
 */
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

static int g_spi = -1;
static int g_ce_fd = -1;

static int gpio_export_out(int pin)
{
    char path[64];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        char buf[8];
        int n = snprintf(buf, sizeof(buf), "%d", pin);
        (void)write(fd, buf, (size_t)n);
        close(fd);
        usleep(100000);
    }
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    for (int i = 0; i < 20; i++) {
        fd = open(path, O_WRONLY);
        if (fd >= 0) {
            break;
        }
        usleep(50000);
    }
    if (fd < 0) {
        return -1;
    }
    write(fd, "out", 3);
    close(fd);
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    return open(path, O_WRONLY);
}

static void ce_low()
{
    if (g_ce_fd >= 0) {
        write(g_ce_fd, "0", 1);
    }
}

static void ce_high()
{
    if (g_ce_fd >= 0) {
        write(g_ce_fd, "1", 1);
    }
}

static int spi_xfer(uint8_t *tx, uint8_t *rx, size_t len)
{
    struct spi_ioc_transfer tr {};
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = (uint32_t)len;
    tr.speed_hz = 1000000;
    tr.bits_per_word = 8;
    return ioctl(g_spi, SPI_IOC_MESSAGE(1), &tr);
}

static uint8_t reg_write(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = {(uint8_t)(0x20U | (reg & 0x1FU)), val};
    uint8_t rx[2] = {};
    spi_xfer(tx, rx, 2);
    return rx[0];
}

static uint8_t reg_read(uint8_t reg)
{
    uint8_t tx[2] = {(uint8_t)(0x00U | (reg & 0x1FU)), 0xFFU};
    uint8_t rx[2] = {};
    spi_xfer(tx, rx, 2);
    return rx[1];
}

static int nrf_init_rx()
{
    g_spi = open("/dev/spidev0.0", O_RDWR);
    if (g_spi < 0) {
        perror("open /dev/spidev0.0");
        return -1;
    }

    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 1000000;
    ioctl(g_spi, SPI_IOC_WR_MODE, &mode);
    ioctl(g_spi, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(g_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    g_ce_fd = gpio_export_out(25);
    if (g_ce_fd < 0) {
        perror("GPIO25 CE");
        return -1;
    }
    ce_low();

    usleep(100000);
    reg_write(0x00, 0x0E);
    reg_write(0x05, 0x4C);
    reg_write(0x06, 0x07);
    reg_write(0x02, 0x01);
    reg_write(0x03, 0x03);
    reg_write(0x04, 0x00);
    reg_write(0x07, 0x70);
    reg_write(0x00, 0x0F);
    reg_write(0x1C, 0x01);
    reg_write(0x1D, 0x01);
    reg_write(0x06, 0x0F);
    ce_high();
    return 0;
}

static int read_payload(uint8_t *buf, size_t cap)
{
    uint8_t st = reg_read(0x07);
    if ((st & 0x40U) == 0U) {
        return 0;
    }
    uint8_t tx[33] = {0x61U};
    uint8_t rx[33] = {};
    spi_xfer(tx, rx, 33);
    size_t n = cap < 32U ? cap : 32U;
    for (size_t i = 0; i < n; i++) {
        buf[i] = rx[i + 1];
    }
    reg_write(0x07, 0x70);
    return (int)n;
}

int main()
{
    setvbuf(stdout, nullptr, _IOLBF, 0);

    if (nrf_init_rx() != 0) {
        return 1;
    }

    uint8_t st0 = reg_read(0x07);
    std::printf("NRF RX init: STATUS=0x%02X (%s)\n", st0,
                (st0 == 0xFF) ? "FAIL no module" : "listening");
    if (st0 == 0xFF) {
        return 2;
    }

    uint8_t buf[64];
    while (true) {
        int n = read_payload(buf, sizeof(buf) - 1U);
        if (n > 0) {
            buf[n] = 0;
            std::printf("RX: %s\n", buf);
            fflush(stdout);
        } else {
            usleep(10000);
        }
    }
    return 0;
}
