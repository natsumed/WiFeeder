/*
 * hx711.c - HX711 load cell amplifier (PB4 DOUT, PB5 SCK bit-bang)
 */
#include "hx711.h"
#include "board.h"
#include <stdbool.h>

static void sck_high(void) { *BOARD_GPIOB_ODR |=  (1U << BOARD_HX711_SCK_PIN); }
static void sck_low(void)  { *BOARD_GPIOB_ODR &= ~(1U << BOARD_HX711_SCK_PIN); }
static uint8_t dout_read(void) { return (uint8_t)((*BOARD_GPIOB_IDR >> BOARD_HX711_DOUT_PIN) & 1U); }

static bool wait_ready(uint32_t timeout_ms)
{
    uint32_t end = board_millis() + timeout_ms;
    while (dout_read() != 0U) {
        if (board_millis() >= end) {
            return false;
        }
    }
    return true;
}

void hx711_init(void)
{
    board_gpio_enable_clocks();

    /* PB4 input, PB5 output */
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER & ~((3U << 8) | (3U << 10))) |
                         (0U << 8) | (1U << 10);
    sck_low();

    /* Channel A, gain 128: extra pulse after 24 bits */
    (void)hx711_read_raw();
}

int32_t hx711_read_raw(void)
{
    uint32_t value = 0;
    int bit;

    if (!wait_ready(200U)) {
        return 0;
    }

    for (bit = 0; bit < 24; bit++) {
        sck_high();
        board_delay(2);
        value = (value << 1) | (uint32_t)dout_read();
        sck_low();
        board_delay(2);
    }

    /* 25th pulse => channel A gain 128 */
    sck_high();
    board_delay(2);
    sck_low();
    board_delay(2);

    if (value & 0x800000U) {
        value |= 0xFF000000U;
    }
    return (int32_t)value;
}

int32_t hx711_read_average(uint8_t n)
{
    int64_t sum = 0;
    uint8_t i;
    uint8_t count = 0;

    if (n == 0U) {
        n = 1U;
    }

    for (i = 0; i < n; i++) {
        int32_t sample = hx711_read_raw();
        sum += sample;
        count++;
        (void)wait_ready(120U);
    }

    return (int32_t)(sum / (int64_t)count);
}
