/*
 * nrf24l01.h - NRF24L01+ driver for WiFeeder v2 (bit-bang SPI)
 *
 * Pin connections (NUCLEO-L432KC):
 *   PA5 = SCK, PA7 = MOSI, PB1 = MISO (pull-up input)
 *   PA4 = CSN, PB0 = CE
 */
#ifndef NRF24L01_H
#define NRF24L01_H

#include <stdint.h>
#include <stdbool.h>

#define NRF_CMD_R_REGISTER      0x00U
#define NRF_CMD_W_REGISTER      0x20U
#define NRF_CMD_R_RX_PAYLOAD    0x61U
#define NRF_CMD_W_TX_PAYLOAD    0xA0U
#define NRF_CMD_FLUSH_TX        0xE1U
#define NRF_CMD_FLUSH_RX        0xE2U
#define NRF_CMD_NOP             0xFFU

#define NRF_REG_CONFIG          0x00U
#define NRF_REG_EN_AA           0x01U
#define NRF_REG_EN_RXADDR       0x02U
#define NRF_REG_SETUP_AW        0x03U
#define NRF_REG_SETUP_RETR      0x04U
#define NRF_REG_RF_CH           0x05U
#define NRF_REG_RF_SETUP        0x06U
#define NRF_REG_STATUS          0x07U
#define NRF_REG_RX_ADDR_P0      0x0AU
#define NRF_REG_TX_ADDR         0x10U
#define NRF_REG_RX_PW_P0        0x11U
#define NRF_REG_FIFO_STATUS     0x17U

#define NRF_STATUS_RX_DR        0x40U
#define NRF_STATUS_TX_DS        0x20U
#define NRF_STATUS_MAX_RT       0x10U

#define NRF_PAYLOAD_SIZE        32U
#define NRF_ADDR_WIDTH          5U

void nrf24_init(void);
uint8_t nrf24_read_reg(uint8_t reg);
void nrf24_write_reg(uint8_t reg, uint8_t value);
bool nrf24_is_connected(void);
void nrf24_set_channel(uint8_t channel);
void nrf24_config_esb(void);
bool nrf24_send(const uint8_t *data, uint8_t len);
bool nrf24_recv(uint8_t *data, uint8_t len);
bool nrf24_is_data_available(void);
void nrf24_set_mode_rx(void);
void nrf24_set_mode_tx(void);

#endif /* NRF24L01_H */
