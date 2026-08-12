/*
 * encoder.c - TIM2 quadrature encoder on PA0/PA1 (motor 1)
 *
 * Motor 2 encoder (PA11/PA12) is Phase 3b; returns 0 for MOTOR_2.
 */
#include "encoder.h"
#include "board.h"

#define ENCODER_TIM2_BASE   0x40000000U

static volatile uint32_t *const tim2_cr1    = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x00U);
static volatile uint32_t *const tim2_smcr   = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x08U);
static volatile uint32_t *const tim2_ccmr1  = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x18U);
static volatile uint32_t *const tim2_ccer   = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x20U);
static volatile uint32_t *const tim2_cnt    = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x24U);
static volatile uint32_t *const tim2_psc    = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x28U);
static volatile uint32_t *const tim2_arr    = (volatile uint32_t *)(ENCODER_TIM2_BASE + 0x2CU);

static int32_t gs_enc1_offset;

void encoder_init(void)
{
    board_gpio_enable_clocks();
    *BOARD_RCC_APB1ENR1 |= (1U << 0); /* TIM2 */
    board_delay(10);

    /* PA0/PA1 AF1 TIM2_CH1/CH2 */
    *BOARD_GPIOA_MODER = (*BOARD_GPIOA_MODER & ~((3U << 0) | (3U << 2))) |
                         (2U << 0) | (2U << 2);
    *BOARD_GPIOA_AFRL = (*BOARD_GPIOA_AFRL & ~0xFFU) | 0x11U;

    *tim2_psc = 0;
    *tim2_arr = 0xFFFFU;
    *tim2_ccmr1 = 0x1111U; /* CC1S/CC2S encoder mode on TI1/TI2 */
    *tim2_ccer = (1U << 1) | (1U << 5); /* CC1P, CC2P inverted if needed */
    *tim2_smcr = (3U << 0) | (1U << 4); /* SMS=011 encoder, TS=001 TI1FP1 */
    *tim2_cnt = 0;
    *tim2_cr1 |= (1U << 0);
    gs_enc1_offset = 0;
}

int32_t encoder_get_count(motor_id_t motor)
{
    if (motor == MOTOR_1) {
        return (int32_t)(*tim2_cnt) - gs_enc1_offset;
    }
    return 0;
}

void encoder_reset(motor_id_t motor)
{
    if (motor == MOTOR_1) {
        gs_enc1_offset = (int32_t)(*tim2_cnt);
    }
}
