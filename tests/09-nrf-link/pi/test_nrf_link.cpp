/*
 * Test 09 — Pi NRF link client (no Auto-ACK, matches STM peer)
 * Adapter VCC = 5V (AM1117). CE = GPIO25 (pin 22).
 */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <time.h>

static int g_spi = -1;
static int g_ce_fd = -1;

static const uint8_t ADDR[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
static const uint8_t RF_CH = 76;
/* 1 Mbps + PA_MAX + LNA enable — required for SI24R1 / PA+LNA (RF24 lnaEnable=true) */
static const uint8_t RF_SETUP = 0x07;
static const size_t PAYLOAD_LEN = 8;
static const uint8_t CONFIG_RX = 0x0F;
static const uint8_t CONFIG_TX = 0x0E;

static int gpio_export_out(int pin)
{
    char path[64];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        char buf[8];
        int n = snprintf(buf, sizeof(buf), "%d", pin);
        (void)write(fd, buf, (size_t)n);
        close(fd);
    }
    usleep(100000);
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
    uint8_t tx[2] = {(uint8_t)(reg & 0x1FU), 0xFFU};
    uint8_t rx[2] = {};
    spi_xfer(tx, rx, 2);
    return rx[1];
}

static void write_buf(uint8_t reg, const uint8_t *buf, size_t len)
{
    uint8_t tx[8] = {(uint8_t)(0x20U | (reg & 0x1FU))};
    uint8_t rx[8] = {};
    memcpy(tx + 1, buf, len);
    spi_xfer(tx, rx, 1 + len);
}

static void cmd(uint8_t c)
{
    uint8_t tx[1] = {c};
    uint8_t rx[1] = {};
    spi_xfer(tx, rx, 1);
}

static void write_payload(const uint8_t *buf, size_t len)
{
    uint8_t tx[33] = {0xA0};
    uint8_t rx[33] = {};
    memcpy(tx + 1, buf, len);
    spi_xfer(tx, rx, 1 + len);
}

static void read_payload(uint8_t *buf, size_t len)
{
    uint8_t tx[33] = {0x61};
    uint8_t rx[33] = {};
    memset(tx + 1, 0xFF, len);
    spi_xfer(tx, rx, 1 + len);
    memcpy(buf, rx + 1, len);
}

static void set_rx()
{
    ce_low();
    reg_write(0x00, CONFIG_RX);
    usleep(5000);
    ce_high();
    usleep(5000);
}

static void set_tx()
{
    ce_low();
    reg_write(0x00, CONFIG_TX);
    usleep(5000);
}

static int nrf_init()
{
    g_spi = open("/dev/spidev0.0", O_RDWR);
    if (g_spi < 0) {
        perror("open /dev/spidev0.0");
        return -1;
    }
    uint8_t mode = 0, bits = 8;
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
    usleep(200000);

    cmd(0xE1);
    cmd(0xE2);
    reg_write(0x07, 0x70);
    {
        uint8_t tx[2] = {0x50, 0x73};
        uint8_t rx[2] = {};
        spi_xfer(tx, rx, 2);
    }
    reg_write(0x1D, 0x00);
    reg_write(0x1C, 0x00);
    reg_write(0x00, 0x0C);
    reg_write(0x01, 0x00); /* no AA */
    reg_write(0x02, 0x01);
    reg_write(0x03, 0x03);
    reg_write(0x04, 0x00);
    reg_write(0x05, RF_CH);
    reg_write(0x06, RF_SETUP);
    reg_write(0x11, (uint8_t)PAYLOAD_LEN);
    write_buf(0x0A, ADDR, 5);
    write_buf(0x10, ADDR, 5);

    uint8_t st = reg_read(0x07);
    uint8_t ch = reg_read(0x05);
    uint8_t setup = reg_read(0x06);
    uint8_t aa = reg_read(0x01);
    std::printf("Pi NRF: STATUS=0x%02X CH=%u SETUP=0x%02X EN_AA=0x%02X (expect AA=0)\n",
                st, ch, setup, aa);
    if (st == 0xFF || ch != RF_CH || aa != 0) {
        std::printf("FAIL: local NRF config\n");
        return -1;
    }
    return 0;
}

static uint64_t now_ms()
{
    struct timespec ts {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static bool wait_pong(uint8_t seq, int timeout_ms)
{
    set_rx();
    const uint64_t t0 = now_ms();
    while ((now_ms() - t0) < (uint64_t)timeout_ms) {
        uint8_t st = reg_read(0x07);
        uint8_t fifo = reg_read(0x17);
        if ((st & 0x40) || ((fifo & 0x01) == 0)) {
            uint8_t rx[PAYLOAD_LEN] = {};
            read_payload(rx, PAYLOAD_LEN);
            reg_write(0x07, 0x70);
            cmd(0xE2);
            if (rx[0] == 'P' && rx[1] == 'O' && rx[2] == 'N' && rx[3] == 'G' && rx[4] == seq) {
                return true;
            }
        }
        usleep(1000);
    }
    return false;
}

static uint8_t send_one(uint8_t seq)
{
    uint8_t pkt[PAYLOAD_LEN] = {};
    pkt[0] = 'P';
    pkt[1] = 'I';
    pkt[2] = 'N';
    pkt[3] = 'G';
    pkt[4] = seq;

    set_tx();
    cmd(0xE1);
    reg_write(0x07, 0x70);
    write_payload(pkt, PAYLOAD_LEN);
    ce_high();
    usleep(500);
    uint8_t st = reg_read(0x07);
    ce_low();
    usleep(500);
    reg_write(0x07, 0x70);
    return st;
}

static void send_ping(uint8_t seq)
{
    uint8_t st = 0;
    /* ~1.5s burst to land inside STM's 2s quiet RX window */
    for (int i = 0; i < 200; i++) {
        st = send_one(seq);
        usleep(5000);
    }
    std::printf("[TX=0x%02X%s] ", st, (st & 0x20) ? " TX_DS" : "");
}

int main(int argc, char **argv)
{
    setvbuf(stdout, nullptr, _IOLBF, 0);
    if (nrf_init() != 0) {
        return 1;
    }

    int rounds = 10;
    if (argc > 1) {
        rounds = atoi(argv[1]);
        if (rounds < 1) {
            rounds = 1;
        }
    }

    int ok = 0;
    for (int i = 0; i < rounds; i++) {
        uint8_t seq = (uint8_t)(i & 0xFF);
        std::printf("PING #%d ... ", i);
        fflush(stdout);
        send_ping(seq);
        if (wait_pong(seq, 2000)) {
            std::printf("PONG OK\n");
            ok++;
        } else {
            std::printf("TIMEOUT\n");
        }
        usleep(50000);
    }

    std::printf("\nResult: %d/%d\n", ok, rounds);
    if (ok == rounds) {
        std::printf("PASS: STM32 <-> Pi NRF link OK\n");
        return 0;
    }
    if (ok > 0) {
        std::printf("PARTIAL\n");
        return 3;
    }
    std::printf("FAIL: check adapter VCC~5V with meter; GND; move modules to ~1m\n");
    return 2;
}
