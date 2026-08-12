#ifndef FLASH_INT_H
#define FLASH_INT_H

#include <stdint.h>
#include <stdbool.h>

#define FLASH_INT_MAGIC             0x55AA33CCU
#define FLASH_INT_DEVICE_ADDR       0x0803F800U
#define FLASH_INT_MACHINE_TYPE_ADDR 0x0803F400U

bool flash_int_read_id(uint32_t *id_out);
bool flash_int_write_id(uint32_t id);
bool flash_int_read_type(uint32_t *type_out);
bool flash_int_write_type(uint32_t type);

#endif /* FLASH_INT_H */
