/*
 * board.c - Board helpers for NUCLEO-L432KC (register-level timing)
 */
#include "board.h"

static volatile uint32_t s_millis;

void board_gpio_enable_clocks(void)
{
    *BOARD_RCC_AHB2ENR |= (1U << 0) | (1U << 1); /* GPIOA, GPIOB */
    board_delay(100);
}

void board_delay(volatile uint32_t loops)
{
    while (loops--) {
        __asm volatile("nop");
    }
}

/* SysTick @ 1 kHz assuming BOARD_SYSCLK_HZ (default MSI 4 MHz) */
void board_systick_init(void)
{
    volatile uint32_t *SYST_CSR = (volatile uint32_t *)0xE000E010U;
    volatile uint32_t *SYST_RVR = (volatile uint32_t *)0xE000E014U;
    volatile uint32_t *SYST_CVR = (volatile uint32_t *)0xE000E018U;

    *SYST_CSR = 0U;
    *SYST_RVR = (BOARD_SYSCLK_HZ / 1000U) - 1U;
    *SYST_CVR = 0U;
    *SYST_CSR = 7U; /* ENABLE | TICKINT | CLKSOURCE */
    s_millis = 0U;
}

void SysTick_Handler(void)
{
    s_millis++;
}

uint32_t board_millis(void)
{
    return s_millis;
}

void board_led_init(void)
{
    board_gpio_enable_clocks();
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER & ~(3U << (BOARD_LED_PIN * 2U))) |
                         (1U << (BOARD_LED_PIN * 2U));
}

void board_led_set(int on)
{
    if (on) {
        *BOARD_GPIOB_ODR |= (1U << BOARD_LED_PIN);
    } else {
        *BOARD_GPIOB_ODR &= ~(1U << BOARD_LED_PIN);
    }
}
