#include "nrf24l01.h"
#include "config.h"
#include "error.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace wifeeder {

#ifndef RASPBERRY_PI
#define WIFEEDER_NRF_STUB 1
#endif

enum : uint8_t {
    NRF_REG_CONFIG = 0x00,
    NRF_REG_EN_AA = 0x01,
    NRF_REG_EN_RXADDR = 0x02,
    NRF_REG_SETUP_AW = 0x03,
    NRF_REG_SETUP_RETR = 0x04,
    NRF_REG_RF_CH = 0x05,
    NRF_REG_RF_SETUP = 0x06,
    NRF_REG_STATUS = 0x07,
    NRF_REG_RX_ADDR_P0 = 0x0A,
    NRF_REG_TX_ADDR = 0x10,
    NRF_REG_RX_PW_P0 = 0x11,
    NRF_REG_FIFO_STATUS = 0x17,
    NRF_CMD_FLUSH_TX = 0xE1,
    NRF_CMD_FLUSH_RX = 0xE2,
    NRF_CMD_R_RX_PAYLOAD = 0x61,
    NRF_CMD_W_TX_PAYLOAD = 0xA0,
};

static constexpr uint8_t kAddr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

Nrf24L01::Nrf24L01()
    : spi_fd_(-1), ce_fd_(-1), ce_gpio_(NRF_CE_GPIO), stub_mode_(false),
      initialized_(false), last_tx_ok_(true)
{
#ifdef WIFEEDER_NRF_STUB
    stub_mode_ = true;
#endif
}

Nrf24L01::~Nrf24L01()
{
    deinit();
}

int32_t Nrf24L01::init(const std::string& spi_device, int ce_gpio)
{
    ce_gpio_ = ce_gpio;
#ifdef WIFEEDER_NRF_STUB
    stub_mode_ = true;
    initialized_ = true;
    std::cout << "[nrf24] stub mode: SPI=" << spi_device << " CE=GPIO" << ce_gpio << std::endl;
    return ENOERR;
#else
    spi_fd_ = ::open(spi_device.c_str(), O_RDWR);
    if (spi_fd_ < 0) {
        std::cerr << "[nrf24] open " << spi_device << " failed: " << std::strerror(errno) << std::endl;
        stub_mode_ = true;
        initialized_ = true;
        return EHW;
    }

    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 2000000;
    ::ioctl(spi_fd_, SPI_IOC_WR_MODE, &mode);
    ::ioctl(spi_fd_, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ::ioctl(spi_fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    if (gpio_export(ce_gpio_) != ENOERR || gpio_set_direction(ce_gpio_, true) != ENOERR) {
        std::cerr << "[nrf24] GPIO setup failed, falling back to stub" << std::endl;
        stub_mode_ = true;
    }

    ce_low();
    spi_write_reg(NRF_REG_CONFIG, 0x0E);
    spi_write_reg(NRF_REG_EN_AA, 0x01);
    spi_write_reg(NRF_REG_EN_RXADDR, 0x01);
    spi_write_reg(NRF_REG_SETUP_AW, 0x03);
    spi_write_reg(NRF_REG_SETUP_RETR, 0x1A);
    spi_write_reg(NRF_REG_RF_CH, NRF_RF_CHANNEL);
    spi_write_reg(NRF_REG_RF_SETUP, 0x06);
    spi_write_buf(NRF_REG_TX_ADDR, kAddr, sizeof(kAddr));
    spi_write_buf(NRF_REG_RX_ADDR_P0, kAddr, sizeof(kAddr));
    spi_write_reg(NRF_REG_RX_PW_P0, NRF_PAYLOAD_SIZE);

    initialized_ = true;
    return ENOERR;
#endif
}

void Nrf24L01::deinit()
{
    if (spi_fd_ >= 0) {
        ::close(spi_fd_);
        spi_fd_ = -1;
    }
    if (ce_fd_ >= 0) {
        ::close(ce_fd_);
        ce_fd_ = -1;
    }
    initialized_ = false;
}

int32_t Nrf24L01::set_mode_rx()
{
    if (stub_mode_) {
        return ENOERR;
    }
    ce_low();
    uint8_t cfg = spi_read_reg(NRF_REG_CONFIG);
    cfg = static_cast<uint8_t>((cfg & static_cast<uint8_t>(~0x01U)) | 0x03U);
    spi_write_reg(NRF_REG_CONFIG, cfg);
    ce_high();
    return ENOERR;
}

int32_t Nrf24L01::set_mode_tx()
{
    if (stub_mode_) {
        return ENOERR;
    }
    ce_low();
    uint8_t cfg = spi_read_reg(NRF_REG_CONFIG);
    cfg = static_cast<uint8_t>((cfg & static_cast<uint8_t>(~0x01U)) | 0x02U);
    spi_write_reg(NRF_REG_CONFIG, cfg);
    ce_high();
    return ENOERR;
}

bool Nrf24L01::data_ready() const
{
    if (stub_mode_ || !initialized_) {
        return false;
    }
    return (spi_read_reg(NRF_REG_STATUS) & 0x40U) != 0U;
}

int32_t Nrf24L01::read_payload(uint8_t* buf, size_t len)
{
    if (buf == nullptr || len == 0) {
        return EARG;
    }
    if (stub_mode_) {
        return EWOULDBLOCK;
    }
    uint8_t cmd = NRF_CMD_R_RX_PAYLOAD;
    uint8_t rx[NRF_PAYLOAD_SIZE + 1] = {0};
    struct spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<unsigned long>(&cmd);
    tr.rx_buf = reinterpret_cast<unsigned long>(rx);
    tr.len = static_cast<uint32_t>(len + 1U);
    if (::ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &tr) < 0) {
        return EHW;
    }
    std::memcpy(buf, &rx[1], len);
    return static_cast<int32_t>(len);
}

int32_t Nrf24L01::write_payload(const uint8_t* buf, size_t len)
{
    if (buf == nullptr || len == 0 || len > NRF_PAYLOAD_SIZE) {
        return EARG;
    }
    if (stub_mode_) {
        std::cout << "[nrf24] TX stub " << len << " bytes: ";
        for (size_t i = 0; i < len; ++i) {
            std::cout << std::hex << static_cast<int>(buf[i]) << ' ';
        }
        std::cout << std::dec << std::endl;
        last_tx_ok_ = true;
        return static_cast<int32_t>(len);
    }

    std::vector<uint8_t> tx(len + 1U);
    tx[0] = NRF_CMD_W_TX_PAYLOAD;
    std::memcpy(&tx[1], buf, len);
    struct spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<unsigned long>(tx.data());
    tr.rx_buf = 0;
    tr.len = static_cast<uint32_t>(tx.size());
    if (::ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &tr) < 0) {
        last_tx_ok_ = false;
        return EHW;
    }
    usleep(2000);
    last_tx_ok_ = (spi_read_reg(NRF_REG_STATUS) & 0x20U) == 0U;
    return static_cast<int32_t>(len);
}

bool Nrf24L01::tx_ok() const
{
    return last_tx_ok_;
}

bool Nrf24L01::tx_failed() const
{
    return !last_tx_ok_;
}

void Nrf24L01::flush_tx()
{
    if (!stub_mode_ && spi_fd_ >= 0) {
        spi_transfer(NRF_CMD_FLUSH_TX, 0);
    }
}

void Nrf24L01::flush_rx()
{
    if (!stub_mode_ && spi_fd_ >= 0) {
        spi_transfer(NRF_CMD_FLUSH_RX, 0);
    }
}

int32_t Nrf24L01::gpio_export(int pin)
{
    char path[64];
    std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);
    if (::access(path, F_OK) == 0) {
        return ENOERR;
    }
    int fd = ::open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        return EHW;
    }
    char buf[16];
    const int n = std::snprintf(buf, sizeof(buf), "%d", pin);
    if (::write(fd, buf, static_cast<size_t>(n)) < 0) {
        ::close(fd);
        return EHW;
    }
    ::close(fd);
    usleep(100000);
    return ENOERR;
}

int32_t Nrf24L01::gpio_set_direction(int pin, bool output)
{
    char path[64];
    std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    ce_fd_ = ::open(path, O_WRONLY);
    if (ce_fd_ < 0) {
        return EHW;
    }
    const char* dir = output ? "out" : "in";
    if (::write(ce_fd_, dir, std::strlen(dir)) < 0) {
        return EHW;
    }
    return ENOERR;
}

int32_t Nrf24L01::gpio_write(int pin, bool high)
{
    (void)pin;
    if (stub_mode_) {
        return ENOERR;
    }
    char path[64];
    std::snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", ce_gpio_);
    int fd = ::open(path, O_WRONLY);
    if (fd < 0) {
        return EHW;
    }
    const char val = high ? '1' : '0';
    if (::write(fd, &val, 1) < 0) {
        ::close(fd);
        return EHW;
    }
    ::close(fd);
    return ENOERR;
}

uint8_t Nrf24L01::spi_transfer(uint8_t reg, uint8_t value) const
{
    if (spi_fd_ < 0) {
        return 0;
    }
    uint8_t tx[2] = {reg, value};
    uint8_t rx[2] = {0};
    struct spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<unsigned long>(tx);
    tr.rx_buf = reinterpret_cast<unsigned long>(rx);
    tr.len = 2;
    ::ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &tr);
    return rx[1];
}

uint8_t Nrf24L01::spi_read_reg(uint8_t reg) const
{
    return spi_transfer(static_cast<uint8_t>(reg), 0xFF);
}

void Nrf24L01::spi_write_reg(uint8_t reg, uint8_t value)
{
    spi_transfer(static_cast<uint8_t>(0x20U | reg), value);
}

void Nrf24L01::spi_write_buf(uint8_t reg, const uint8_t* data, size_t len)
{
    if (data == nullptr || len == 0 || spi_fd_ < 0) {
        return;
    }
    std::vector<uint8_t> tx(len + 1U);
    tx[0] = static_cast<uint8_t>(0x20U | reg);
    std::memcpy(&tx[1], data, len);
    struct spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<unsigned long>(tx.data());
    tr.rx_buf = 0;
    tr.len = static_cast<uint32_t>(tx.size());
    ::ioctl(spi_fd_, SPI_IOC_MESSAGE(1), &tr);
}

void Nrf24L01::ce_high()
{
    gpio_write(ce_gpio_, true);
}

void Nrf24L01::ce_low()
{
    gpio_write(ce_gpio_, false);
}

} /* namespace wifeeder */
