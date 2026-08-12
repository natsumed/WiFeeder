/*
 * flash_int.c - Device ID / machine type (RAM-backed with optional flash later)
 *
 * Production can persist via HAL flash erase/program; for bring-up we keep
 * values in RAM so firmware does not depend on HAL_Init reconfiguring SysTick.
 */
#include "flash_int.h"
#include "crc8.h"
#include <string.h>

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t value;
    uint8_t  crc;
} flash_int_record_t;

static uint32_t gs_ram_device_id = 2U;
static uint32_t gs_ram_machine_type = 1U; /* DAC */
static bool gs_ram_device_valid = true;
static bool gs_ram_type_valid = true;

static bool record_valid(const flash_int_record_t *rec)
{
    if (rec->magic != FLASH_INT_MAGIC) {
        return false;
    }
    return crc8_calc((const uint8_t *)&rec->magic, 8U) == rec->crc;
}

bool flash_int_read_id(uint32_t *id_out)
{
    /* Try flash image at programmed address (unprogrammed = 0xFF → invalid) */
    flash_int_record_t rec;
    memcpy(&rec, (const void *)FLASH_INT_DEVICE_ADDR, sizeof(rec));
    if (record_valid(&rec)) {
        *id_out = rec.value;
        gs_ram_device_id = rec.value;
        gs_ram_device_valid = true;
        return true;
    }
    if (gs_ram_device_valid) {
        *id_out = gs_ram_device_id;
        return true;
    }
    return false;
}

bool flash_int_write_id(uint32_t id)
{
    gs_ram_device_id = id;
    gs_ram_device_valid = true;
    /* Persistent flash write deferred until HAL clock/flash bring-up is complete */
    return true;
}

bool flash_int_read_type(uint32_t *type_out)
{
    flash_int_record_t rec;
    memcpy(&rec, (const void *)FLASH_INT_MACHINE_TYPE_ADDR, sizeof(rec));
    if (record_valid(&rec)) {
        *type_out = rec.value;
        gs_ram_machine_type = rec.value;
        gs_ram_type_valid = true;
        return true;
    }
    if (gs_ram_type_valid) {
        *type_out = gs_ram_machine_type;
        return true;
    }
    return false;
}

bool flash_int_write_type(uint32_t type)
{
    gs_ram_machine_type = type;
    gs_ram_type_valid = true;
    return true;
}
