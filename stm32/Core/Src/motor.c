/*
 * motor.c - IBT-2 motor control via PCA9685 PWM0/PWM1
 *
 * MVP: Motor1 only (RPWM=ch0, LPWM=ch1). EN tied to 3.3V in hardware.
 * Motor2 deferred (PB6/PB7 owned by I2C).
 */
#include "motor.h"
#include "board.h"
#include "pca9685.h"

static uint16_t gs_motor1_revs;
static uint8_t gs_motor1_duty = 70U;
static bool gs_motor1_forward = true;
static bool gs_pca_ok;

static void apply_m1(uint8_t duty_pct)
{
    if (!gs_pca_ok) {
        return;
    }
    if (duty_pct == 0U) {
        pca9685_set_duty(BOARD_PCA_CH_RPWM, 0);
        pca9685_set_duty(BOARD_PCA_CH_LPWM, 0);
        return;
    }
    if (gs_motor1_forward) {
        pca9685_set_duty(BOARD_PCA_CH_LPWM, 0);
        pca9685_set_duty(BOARD_PCA_CH_RPWM, duty_pct);
    } else {
        pca9685_set_duty(BOARD_PCA_CH_RPWM, 0);
        pca9685_set_duty(BOARD_PCA_CH_LPWM, duty_pct);
    }
}

void motor_init(void)
{
    gs_pca_ok = pca9685_init();
    motor_stop(MOTOR_1);
    motor_stop(MOTOR_2);
}

void motor_set_dir(motor_id_t motor_id, bool forward)
{
    if (motor_id != MOTOR_1) {
        return; /* Motor2 deferred */
    }
    gs_motor1_forward = forward;
}

void motor_set_duty(motor_id_t motor_id, uint8_t duty_pct)
{
    if (motor_id != MOTOR_1) {
        return;
    }
    gs_motor1_duty = duty_pct;
    apply_m1(duty_pct);
}

void motor_stop(motor_id_t motor_id)
{
    if (motor_id == MOTOR_1) {
        gs_motor1_revs = 0;
        apply_m1(0);
    }
    /* MOTOR_2: no-op (deferred) */
}

void motor_run(motor_id_t motor_id, uint16_t revs_target)
{
    if (motor_id != MOTOR_1) {
        return; /* Motor2 deferred until second PCA/IBT channel */
    }
    gs_motor1_revs = revs_target;
    motor_set_dir(MOTOR_1, true);
    apply_m1(gs_motor1_duty);
}

bool motor_pca_ok(void)
{
    return gs_pca_ok;
}
