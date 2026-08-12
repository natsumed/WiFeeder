#ifndef CRC8_H
#define CRC8_H

#include <stddef.h>
#include <stdint.h>

#define CRC8_POLY   0x07U
#define CRC8_INIT   0x00U

uint8_t crc8_calc(const uint8_t *data, size_t len);
uint8_t crc8_update(uint8_t crc, uint8_t byte);

#endif /* CRC8_H */
