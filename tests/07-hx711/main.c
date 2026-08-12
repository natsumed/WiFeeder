/* HX711 load cell ADC — PB4 DOUT, PB5 SCK bit-bang, 24-bit read */
void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

#include "../common/test_utils.h"

#define RCC_AHB2ENR   (*(volatile uint32_t *)0x4002104C)
#define GPIOB_MODER   (*(volatile uint32_t *)0x48000400)
#define GPIOB_ODR     (*(volatile uint32_t *)0x48000414)
#define GPIOB_IDR     (*(volatile uint32_t *)0x48000410)

#define HX_DOUT_PIN   4U
#define HX_SCK_PIN    5U
#define HX_DOUT_MASK  (1U << HX_DOUT_PIN)
#define HX_SCK_MASK   (1U << HX_SCK_PIN)

static void hx_gpio_init(void)
{
    RCC_AHB2ENR |= (1U << 1);
    for (volatile uint32_t i = 0; i < 100U; i++) {
    }
    GPIOB_MODER = (GPIOB_MODER & ~((3U << (HX_DOUT_PIN * 2U)) | (3U << (HX_SCK_PIN * 2U))))
                | (0U << (HX_DOUT_PIN * 2U))
                | (1U << (HX_SCK_PIN * 2U));
    GPIOB_ODR &= ~HX_SCK_MASK;
}

static void hx_pulse(void)
{
    GPIOB_ODR |= HX_SCK_MASK;
    for (volatile uint32_t i = 0; i < 20U; i++) {
    }
    GPIOB_ODR &= ~HX_SCK_MASK;
    for (volatile uint32_t i = 0; i < 20U; i++) {
    }
}

static int32_t hx711_read_raw(void)
{
    while ((GPIOB_IDR & HX_DOUT_MASK) != 0U) {
    }

    uint32_t value = 0U;
    for (int i = 0; i < 24; i++) {
        GPIOB_ODR |= HX_SCK_MASK;
        for (volatile uint32_t d = 0; d < 10U; d++) {
        }
        value = (value << 1) | ((GPIOB_IDR & HX_DOUT_MASK) ? 1U : 0U);
        GPIOB_ODR &= ~HX_SCK_MASK;
        for (volatile uint32_t d = 0; d < 10U; d++) {
        }
    }

    hx_pulse();

    if (value & 0x800000U) {
        value |= 0xFF000000U;
    }
    return (int32_t)value;
}

static void blink_magnitude(int32_t raw)
{
    uint32_t mag = (raw < 0) ? (uint32_t)(-raw) : (uint32_t)raw;
    mag = (mag >> 12) & 0x0FU;
    if (mag == 0U) {
        mag = 1U;
    }
    test_blink_pattern(mag, 120U, 120U);
}

int main(void)
{
    hx_gpio_init();
    test_led_init();

    int32_t last = 0;
    while (1) {
        int32_t raw = hx711_read_raw();
        if ((raw > last + 5000) || (raw < last - 5000)) {
            last = raw;
            blink_magnitude(raw);
        }
        test_delay_ms(200U);
    }
}
