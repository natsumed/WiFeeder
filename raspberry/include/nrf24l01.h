#ifndef WIFEEDER_NRF24L01_H
#define WIFEEDER_NRF24L01_H

#include <cstdint>
#include <cstddef>
#include <string>

namespace wifeeder {

class Nrf24L01 {
public:
    Nrf24L01();
    ~Nrf24L01();

    int32_t init(const std::string& spi_device, int ce_gpio);
    void deinit();

    int32_t set_mode_rx();
    int32_t set_mode_tx();
    bool data_ready() const;
    int32_t read_payload(uint8_t* buf, size_t len);
    int32_t write_payload(const uint8_t* buf, size_t len);
    bool tx_ok() const;
    bool tx_failed() const;
    void flush_tx();
    void flush_rx();

    bool is_stub() const { return stub_mode_; }

private:
    int spi_fd_;
    int ce_fd_;
    int ce_gpio_;
    bool stub_mode_;
    bool initialized_;
    mutable bool last_tx_ok_;

    int32_t gpio_export(int pin);
    int32_t gpio_set_direction(int pin, bool output);
    int32_t gpio_write(int pin, bool high);
    uint8_t spi_transfer(uint8_t reg, uint8_t value) const;
    uint8_t spi_read_reg(uint8_t reg) const;
    void spi_write_reg(uint8_t reg, uint8_t value);
    void spi_write_buf(uint8_t reg, const uint8_t* data, size_t len);
    void ce_high();
    void ce_low();
};

} /* namespace wifeeder */

#endif /* WIFEEDER_NRF24L01_H */
