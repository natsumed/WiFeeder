#ifndef WIFTEST_TEST_UTILS_H
#define WIFTEST_TEST_UTILS_H

#include <stdint.h>

/* NUCLEO-L432KC onboard LED LD3 on PB3 (active high). */

void test_led_init(void);
void test_led_on(void);
void test_led_off(void);
void test_led_toggle(void);

/* Busy-loop delay; approximate ms at ~4 MHz reset clock unless HSI reconfigured. */
void test_delay_ms(uint32_t ms);

/* Blink count times: on_ms / off_ms per cycle. */
void test_blink_pattern(uint32_t count, uint32_t on_ms, uint32_t off_ms);

#endif
