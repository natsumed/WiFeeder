/* TIM2 quadrature encoder on PA0/PA1 — register level */
void SystemInit(void) {}
unsigned int SystemCoreClock = 16000000;

#include "../common/test_utils.h"

#define RCC_CR        (*(volatile uint32_t *)0x40021000)
#define RCC_CFGR      (*(volatile uint32_t *)0x40021008)
#define RCC_APB1ENR1  (*(volatile uint32_t *)0x40021058)
#define RCC_AHB2ENR   (*(volatile uint32_t *)0x4002104C)

#define GPIOA_MODER   (*(volatile uint32_t *)0x48000000)
#define GPIOA_PUPDR   (*(volatile uint32_t *)0x4800000C)

#define TIM2_CR1      (*(volatile uint32_t *)0x40000000)
#define TIM2_SMCR     (*(volatile uint32_t *)0x40000008)
#define TIM2_CCMR1    (*(volatile uint32_t *)0x40000018)
#define TIM2_CCER     (*(volatile uint32_t *)0x40000020)
#define TIM2_CNT      (*(volatile uint32_t *)0x40000024)
#define TIM2_ARR      (*(volatile uint32_t *)0x4000002C)

static void clock_hsi_16mhz(void)
{
    RCC_CR |= (1U << 0);
    while ((RCC_CR & (1U << 2)) == 0U) {
    }
    RCC_CFGR = (RCC_CFGR & ~3U);
    while ((RCC_CFGR & (3U << 2)) != 0U) {
    }
}

static void encoder_init(void)
{
    RCC_AHB2ENR |= (1U << 0);
    RCC_APB1ENR1 |= (1U << 0);
    for (volatile uint32_t i = 0; i < 100U; i++) {
    }

    /* PA0/PA1 inputs, pull-up */
    GPIOA_MODER &= ~((3U << 0) | (3U << 2));
    GPIOA_PUPDR = (GPIOA_PUPDR & ~((3U << 0) | (3U << 2))) | (1U << 0) | (1U << 2);

    TIM2_CR1 = 0;
    TIM2_SMCR = (3U << 0) | (1U << 4);
    TIM2_CCMR1 = (1U << 0) | (1U << 8);
    TIM2_CCER = (1U << 0) | (1U << 1) | (1U << 4) | (1U << 5);
    TIM2_ARR = 0xFFFFU;
    TIM2_CNT = 0U;
    TIM2_CR1 = (1U << 0);
}

int main(void)
{
    clock_hsi_16mhz();
    encoder_init();
    test_led_init();

    uint32_t last = TIM2_CNT;
    while (1) {
        uint32_t now = TIM2_CNT;
        if (now != last) {
            last = now;
            test_blink_pattern(1U, 50U, 50U);
        }
        test_delay_ms(10U);
    }
}
