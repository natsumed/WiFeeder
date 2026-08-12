#include "test_utils.h"

/* Register map — bare metal, no HAL. */
#define RCC_AHB2ENR   (*(volatile uint32_t *)0x4002104C)
#define GPIOB_MODER   (*(volatile uint32_t *)0x48000400)
#define GPIOB_ODR     (*(volatile uint32_t *)0x48000414)

#define LED_PIN       3U
#define RCC_GPIOB_EN  (1U << 1)
#define LED_MASK      (1U << LED_PIN)

static uint32_t s_led_ready;

void test_led_init(void)
{
    if (s_led_ready) {
        return;
    }
    RCC_AHB2ENR |= RCC_GPIOB_EN;
    for (volatile uint32_t i = 0; i < 100U; i++) {
    }
    GPIOB_MODER = (GPIOB_MODER & ~(3U << (LED_PIN * 2U))) | (1U << (LED_PIN * 2U));
    GPIOB_ODR &= ~LED_MASK;
    s_led_ready = 1U;
}

void test_led_on(void)
{
    test_led_init();
    GPIOB_ODR |= LED_MASK;
}

void test_led_off(void)
{
    test_led_init();
    GPIOB_ODR &= ~LED_MASK;
}

void test_led_toggle(void)
{
    test_led_init();
    GPIOB_ODR ^= LED_MASK;
}

void test_delay_ms(uint32_t ms)
{
    /* ~4000 cycles/ms at 4 MHz HSI after reset; tune if clock changed. */
    while (ms--) {
        for (volatile uint32_t i = 0; i < 4000U; i++) {
        }
    }
}

void test_blink_pattern(uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    test_led_init();
    while (count--) {
        test_led_on();
        test_delay_ms(on_ms);
        test_led_off();
        test_delay_ms(off_ms);
    }
}
