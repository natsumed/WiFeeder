/*
 * nrf24l01.c - NRF24L01+ bit-bang SPI driver (PA4 CSN, PB1 MISO)
 */
#include "nrf24l01.h"
#include "board.h"
#include <stddef.h>

static volatile uint32_t *const gpioa_moder = BOARD_GPIOA_MODER;
static volatile uint32_t *const gpioa_odr   = BOARD_GPIOA_ODR;
static volatile uint32_t *const gpiob_moder = BOARD_GPIOB_MODER;
static volatile uint32_t *const gpiob_odr   = BOARD_GPIOB_ODR;
static volatile uint32_t *const gpiob_idr   = BOARD_GPIOB_IDR;
static volatile uint32_t *const gpiob_pupdr = BOARD_GPIOB_PUPDR;

static const uint8_t gs_nrf_addr[NRF_ADDR_WIDTH] = {0xE7U, 0xE7U, 0xE7U, 0xE7U, 0xE7U};

static void csn_low(void)  { *gpioa_odr &= ~(1U << BOARD_NRF_CSN_PIN); }
static void csn_high(void) { *gpioa_odr |=  (1U << BOARD_NRF_CSN_PIN); }
static void ce_low(void)   { *gpiob_odr &= ~(1U << BOARD_NRF_CE_PIN); }
static void ce_high(void)  { *gpiob_odr |=  (1U << BOARD_NRF_CE_PIN); }
static void sck_high(void) { *gpioa_odr |=  (1U << BOARD_NRF_SCK_PIN); }
static void sck_low(void)  { *gpioa_odr &= ~(1U << BOARD_NRF_SCK_PIN); }
static void mosi_high(void){ *gpioa_odr |=  (1U << BOARD_NRF_MOSI_PIN); }
static void mosi_low(void) { *gpioa_odr &= ~(1U << BOARD_NRF_MOSI_PIN); }
static uint8_t miso_read(void) { return (uint8_t)((*gpiob_idr >> BOARD_NRF_MISO_PIN) & 1U); }

static uint8_t spi_xfer(uint8_t out)
{
    uint8_t in = 0;
    int bit;

    for (bit = 7; bit >= 0; bit--) {
        if (out & (1U << (uint8_t)bit)) {
            mosi_high();
        } else {
            mosi_low();
        }
        sck_high();
        in = (uint8_t)((in << 1) | miso_read());
        sck_low();
    }
    return in;
}

static void write_buf(uint8_t cmd, const uint8_t *buf, uint8_t len)
{
    uint8_t i;
    csn_low();
    board_delay(10);
    (void)spi_xfer(cmd);
    for (i = 0; i < len; i++) {
        (void)spi_xfer(buf[i]);
    }
    csn_high();
}

static void read_buf(uint8_t cmd, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    csn_low();
    board_delay(10);
    (void)spi_xfer(cmd);
    for (i = 0; i < len; i++) {
        buf[i] = spi_xfer(NRF_CMD_NOP);
    }
    csn_high();
}

static void write_reg_buf(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    write_buf((uint8_t)(NRF_CMD_W_REGISTER | (reg & 0x1FU)), buf, len);
}

static void flush_tx(void)
{
    csn_low();
    board_delay(10);
    (void)spi_xfer(NRF_CMD_FLUSH_TX);
    csn_high();
}

static void flush_rx(void)
{
    csn_low();
    board_delay(10);
    (void)spi_xfer(NRF_CMD_FLUSH_RX);
    csn_high();
}

static void clear_irq(uint8_t flags)
{
    nrf24_write_reg(NRF_REG_STATUS, flags);
}

void nrf24_init(void)
{
    board_gpio_enable_clocks();

    /* PA5 SCK, PA7 MOSI, PA4 CSN outputs; PB0 CE output; PB1 MISO input pull-up */
    *gpioa_moder = (*gpioa_moder & ~((3U << 10) | (3U << 14) | (3U << 8))) |
                   (1U << 10) | (1U << 14) | (1U << 8);
    *gpiob_moder = (*gpiob_moder & ~((3U << 0) | (3U << 2))) | (1U << 0) | (0U << 2);
    *gpiob_pupdr = (*gpiob_pupdr & ~(3U << 2)) | (1U << 2);

    csn_high();
    sck_low();
    ce_low();
    board_delay(50000);
}

uint8_t nrf24_read_reg(uint8_t reg)
{
    uint8_t val;
    csn_low();
    board_delay(10);
    (void)spi_xfer((uint8_t)(NRF_CMD_R_REGISTER | (reg & 0x1FU)));
    val = spi_xfer(NRF_CMD_NOP);
    csn_high();
    return val;
}

void nrf24_write_reg(uint8_t reg, uint8_t value)
{
    csn_low();
    board_delay(10);
    (void)spi_xfer((uint8_t)(NRF_CMD_W_REGISTER | (reg & 0x1FU)));
    (void)spi_xfer(value);
    csn_high();
}

bool nrf24_is_connected(void)
{
    uint8_t status = nrf24_read_reg(NRF_REG_STATUS);
    uint8_t config = nrf24_read_reg(NRF_REG_CONFIG);

    if (status == 0x00U || status == 0xFFU || config == 0x00U || config == 0xFFU) {
        return false;
    }

    nrf24_write_reg(NRF_REG_RF_CH, 76U);
    return (nrf24_read_reg(NRF_REG_RF_CH) == 76U);
}

void nrf24_set_channel(uint8_t channel)
{
    nrf24_write_reg(NRF_REG_RF_CH, channel);
}

void nrf24_config_esb(void)
{
    ce_low();
    flush_tx();
    flush_rx();
    clear_irq(NRF_STATUS_RX_DR | NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT);

    /* Power down while configuring */
    nrf24_write_reg(NRF_REG_CONFIG, 0x0EU);

    nrf24_write_reg(NRF_REG_SETUP_RETR, 0x1AU); /* 500us, 10 retries */
    nrf24_set_channel(76U);
    nrf24_write_reg(NRF_REG_RF_SETUP, 0x06U);   /* 1 Mbps, 0 dBm */
    nrf24_write_reg(NRF_REG_EN_AA, 0x01U);
    nrf24_write_reg(NRF_REG_EN_RXADDR, 0x01U);
    nrf24_write_reg(NRF_REG_SETUP_AW, 0x03U);   /* 5-byte address */
    nrf24_write_reg(NRF_REG_RX_PW_P0, NRF_PAYLOAD_SIZE);

    write_reg_buf(NRF_REG_TX_ADDR, gs_nrf_addr, NRF_ADDR_WIDTH);
    write_reg_buf(NRF_REG_RX_ADDR_P0, gs_nrf_addr, NRF_ADDR_WIDTH);

    nrf24_set_mode_rx();
}

void nrf24_set_mode_rx(void)
{
    uint8_t cfg = nrf24_read_reg(NRF_REG_CONFIG);
    cfg = (uint8_t)((cfg & (uint8_t)~0x02U) | 0x03U); /* PWR_UP | PRIM_RX */
    nrf24_write_reg(NRF_REG_CONFIG, cfg);
    ce_high();
    board_delay(200);
}

void nrf24_set_mode_tx(void)
{
    uint8_t cfg = nrf24_read_reg(NRF_REG_CONFIG);
    cfg = (uint8_t)((cfg & (uint8_t)~0x01U) | 0x02U); /* PWR_UP, TX mode */
    nrf24_write_reg(NRF_REG_CONFIG, cfg);
    ce_high();
    board_delay(200);
}

bool nrf24_is_data_available(void)
{
    uint8_t status = nrf24_read_reg(NRF_REG_STATUS);
    if (status & NRF_STATUS_RX_DR) {
        return true;
    }
    return ((nrf24_read_reg(NRF_REG_FIFO_STATUS) & 0x01U) == 0U);
}

bool nrf24_recv(uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0U || len > NRF_PAYLOAD_SIZE) {
        return false;
    }
    if (!nrf24_is_data_available()) {
        return false;
    }
    read_buf(NRF_CMD_R_RX_PAYLOAD, data, len);
    clear_irq(NRF_STATUS_RX_DR);
    return true;
}

bool nrf24_send(const uint8_t *data, uint8_t len)
{
    uint32_t timeout;
    uint8_t status;

    if (data == NULL || len == 0U || len > NRF_PAYLOAD_SIZE) {
        return false;
    }

    nrf24_set_mode_tx();
    ce_low();
    write_buf(NRF_CMD_W_TX_PAYLOAD, data, len);
    ce_high();
    board_delay(20);

    timeout = board_millis() + 50U;
    while (board_millis() < timeout) {
        status = nrf24_read_reg(NRF_REG_STATUS);
        if (status & (NRF_STATUS_TX_DS | NRF_STATUS_MAX_RT)) {
            break;
        }
    }

    ce_low();
    status = nrf24_read_reg(NRF_REG_STATUS);
    if (status & NRF_STATUS_MAX_RT) {
        clear_irq(NRF_STATUS_MAX_RT);
        flush_tx();
        nrf24_set_mode_rx();
        return false;
    }
    if (status & NRF_STATUS_TX_DS) {
        clear_irq(NRF_STATUS_TX_DS);
        nrf24_set_mode_rx();
        return true;
    }

    nrf24_set_mode_rx();
    return false;
}
