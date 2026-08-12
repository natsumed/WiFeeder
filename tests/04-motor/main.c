/*
 * Test 04 — Motor1 via PCA9685 → IBT-2
 *
 * Wiring (see wiring/CONNECTOR_MAP.md / blocks/03-motor.fzz):
 *   PB6/D5 = SCL, PB7/D4 = SDA → PCA9685
 *   PCA PWM0 → IBT RPWM, PWM1 → IBT LPWM
 *   IBT R_EN/L_EN → 3.3V, OE → GND (Buck GND)
 *   IBT M+/M− → GX-2 → motor
 *
 * LED: 3 blinks = PCA OK; continuous slow blink = spinning; fast blink = PCA fail.
 */
void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

#include "board.h"
#include "motor.h"
#include "pca9685.h"
#include "../common/test_utils.h"

int main(void)
{
    board_systick_init();
    board_gpio_enable_clocks();
    board_led_init();
    motor_init();

    if (!motor_pca_ok()) {
        while (1) {
            board_led_set(1);
            test_delay_ms(100U);
            board_led_set(0);
            test_delay_ms(100U);
        }
    }

    test_blink_pattern(3U, 150U, 150U);

    while (1) {
        motor_set_dir(MOTOR_1, true);
        motor_set_duty(MOTOR_1, 60U);
        board_led_set(1);
        test_delay_ms(2000U);

        motor_stop(MOTOR_1);
        board_led_set(0);
        test_delay_ms(1000U);

        motor_set_dir(MOTOR_1, false);
        motor_set_duty(MOTOR_1, 60U);
        board_led_set(1);
        test_delay_ms(2000U);

        motor_stop(MOTOR_1);
        board_led_set(0);
        test_delay_ms(1000U);
    }
}
