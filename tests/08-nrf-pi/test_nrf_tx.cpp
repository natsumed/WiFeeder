/*
 * Pi-side NRF24L01+ TX smoke test via Linux spidev + GPIO CE.
 * Wiring: SPI0 SCLK=11, MISO=9, MOSI=10, CE0=8; CE (radio)=GPIO25
 */
#include <cstdint>
#include <cstdio>
#include <cstring>
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
        (void)write(fd, buf, (size_t)n); /* EBUSY if already exported is OK */
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

static int nrf_init()
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
    reg_write(0x00, 0x0E);       /* PWR_UP | CRC */
    usleep(2000);
    reg_write(0x05, 0x4C);       /* RF_CH = 76 */
    reg_write(0x06, 0x06);       /* 1 Mbps, 0 dBm */
    reg_write(0x07, 0x70);       /* clear IRQ flags */
    return 0;
}

static void send_payload(const uint8_t *data, size_t len)
{
    ce_low();
    reg_write(0x07, 0x70);
    uint8_t tx[33] = {0xA0U};
    uint8_t rx[33] = {};
    memcpy(tx + 1, data, len);
    spi_xfer(tx, rx, 1U + len);
    ce_high();
    usleep(1000);
    ce_low();
}

int main()
{
    setvbuf(stdout, nullptr, _IOLBF, 0);

    if (nrf_init() != 0) {
        return 1;
    }

    /* Force RF_CH write/read check (same as STM test 03) */
    reg_write(0x05, 0x4C);
    uint8_t st0 = reg_read(0x07);
    uint8_t ch = reg_read(0x05);
    std::printf("NRF init: STATUS=0x%02X RF_CH=0x%02X\n", st0, ch);
    if (st0 == 0xFF || ch != 0x4C) {
        std::printf("FAIL: expected STATUS!=0xFF and RF_CH=0x4C (got STATUS=0x%02X RF_CH=0x%02X)\n",
                    st0, ch);
        std::printf("Check: NRF VCC=3.3V, wiring SPI0 pins 19/21/23, CSN pin24, CE GPIO25 pin22\n");
        return 2;
    }
    std::printf("PASS: NRF SPI OK\n");

    const uint8_t msg[] = "WIFTEST_TX";
    for (int i = 0; i < 20; i++) {
        send_payload(msg, sizeof(msg));
        uint8_t st = reg_read(0x07);
        std::printf("TX #%d STATUS=0x%02X\n", i, st);
        fflush(stdout);
        sleep(1);
    }
    close(g_spi);
    if (g_ce_fd >= 0) {
        close(g_ce_fd);
    }
    return 0;
}
