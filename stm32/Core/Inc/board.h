/*
 * board.h - NUCLEO-L432KC pin map for WiFeeder v2 MVP (PCA9685 path)
 *
 * Authoritative wiring: wiring/CONNECTOR_MAP.md
 * One jumper per Nucleo header pin; PB6/PB7 = I2C1 only (not TIM16).
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

/* --- NRF24L01+ (bit-bang SPI) --- */
#define BOARD_NRF_SCK_PIN       5U   /* PA5 */
#define BOARD_NRF_MOSI_PIN      7U   /* PA7 */
#define BOARD_NRF_MISO_PIN      1U   /* PB1, pull-up input — PA6 is shorted */
#define BOARD_NRF_CSN_PIN       4U   /* PA4 */
#define BOARD_NRF_CE_PIN        0U   /* PB0 */

/* --- PCA9685 I2C1 (bit-bang) --- */
#define BOARD_I2C_SCL_PIN       6U   /* PB6 = D5 */
#define BOARD_I2C_SDA_PIN       7U   /* PB7 = D4 */
#define BOARD_PCA9685_ADDR      0x40U

/* PCA9685 PWM channels → IBT-2 (MVP: one motor) */
#define BOARD_PCA_CH_RPWM       0U   /* PWM0 → IBT RPWM */
#define BOARD_PCA_CH_LPWM       1U   /* PWM1 → IBT LPWM */

/* --- Motor2 TIM16 path deferred (pins reused by I2C) --- */
#if 0
#define BOARD_M2_PWM_PIN        6U   /* was PB6 TIM16_CH1 */
#define BOARD_M2_DIR_PIN        7U   /* was PB7 DIR */
#endif

/* --- Encoder 1 (TIM2 quadrature, 4×) --- */
#define BOARD_ENC1_A_PIN        0U   /* PA0 */
#define BOARD_ENC1_B_PIN        1U   /* PA1 */

/* --- RFID reader (USART2 RX) — deferred in MVP wiring --- */
#define BOARD_RFID_RX_PIN       3U   /* PA3 */

/* --- HX711 load cell — deferred in MVP wiring --- */
#define BOARD_HX711_DOUT_PIN    4U   /* PB4 */
#define BOARD_HX711_SCK_PIN     5U   /* PB5 */

/* --- Status LED --- */
#define BOARD_LED_PIN           3U   /* PB3 */

/* --- Timing / clocks --- */
#define BOARD_SYSCLK_HZ         4000000U
#define BOARD_ENCODER_PPR       600U  /* GTS06-OC-RA600A-2M physical P/R */
#define BOARD_ENCODER_COUNTS_PER_REV  (BOARD_ENCODER_PPR * 4U) /* TIM2 SMS=011 x4 */

/* MVP: RFID/HX711 not on Fritzing pack; gate optional features */
#ifndef WIFEEDER_MVP
#define WIFEEDER_MVP            1
#endif

/* Register-level GPIO helpers (match Core/Src/main.c style) */
#define BOARD_GPIOA_MODER       ((volatile uint32_t *)0x48000000U)
#define BOARD_GPIOA_ODR         ((volatile uint32_t *)0x48000014U)
#define BOARD_GPIOA_IDR         ((volatile uint32_t *)0x48000010U)
#define BOARD_GPIOA_AFRL        ((volatile uint32_t *)0x48000020U)
#define BOARD_GPIOA_AFRH        ((volatile uint32_t *)0x48000024U)

#define BOARD_GPIOB_MODER       ((volatile uint32_t *)0x48000400U)
#define BOARD_GPIOB_OTYPER      ((volatile uint32_t *)0x48000404U)
#define BOARD_GPIOB_ODR         ((volatile uint32_t *)0x48000414U)
#define BOARD_GPIOB_IDR         ((volatile uint32_t *)0x48000410U)
#define BOARD_GPIOB_PUPDR       ((volatile uint32_t *)0x4800040CU)
#define BOARD_GPIOB_AFRL        ((volatile uint32_t *)0x48000420U)

#define BOARD_RCC_AHB2ENR       ((volatile uint32_t *)0x4002104CU)
#define BOARD_RCC_APB1ENR1      ((volatile uint32_t *)0x40021058U)
#define BOARD_RCC_APB2ENR       ((volatile uint32_t *)0x40021060U)

void board_gpio_enable_clocks(void);
void board_delay(volatile uint32_t loops);
uint32_t board_millis(void);
void board_systick_init(void);
void board_led_init(void);
void board_led_set(int on);

#endif /* BOARD_H */
