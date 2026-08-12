/*
 * pca9685.h - Adafruit PCA9685 16-ch PWM (I2C bit-bang on PB6/PB7)
 */
#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>
#include <stdbool.h>

#define PCA9685_MODE1           0x00U
#define PCA9685_PRESCALE        0xFEU
#define PCA9685_LED0_ON_L       0x06U

bool pca9685_init(void);
void pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off);
void pca9685_set_duty(uint8_t channel, uint8_t duty_pct);
void pca9685_all_off(void);

#endif /* PCA9685_H */
