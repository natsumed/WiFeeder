/*
 * pca9685.c - PCA9685 over bit-bang I2C (PB6=SCL, PB7=SDA)
 *
 * OE is tied to GND in hardware (wiring/CONNECTOR_MAP.md).
 * Default addr 0x40. PWM ~1 kHz for IBT-2.
 */
#include "pca9685.h"
#include "board.h"

#define I2C_DELAY_LOOPS         40U
#define PCA9685_PWM_HZ          1000U
#define PCA9685_OSC_HZ          25000000U

static void i2c_delay(void)
{
    board_delay(I2C_DELAY_LOOPS);
}

/* Open-drain: drive low as output; release high as input + pull-up */
static void scl_low(void)
{
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER & ~(3U << (BOARD_I2C_SCL_PIN * 2U))) |
                         (1U << (BOARD_I2C_SCL_PIN * 2U));
    *BOARD_GPIOB_ODR &= ~(1U << BOARD_I2C_SCL_PIN);
}

static void scl_high(void)
{
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER & ~(3U << (BOARD_I2C_SCL_PIN * 2U)));
}

static void sda_low(void)
{
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER & ~(3U << (BOARD_I2C_SDA_PIN * 2U))) |
                         (1U << (BOARD_I2C_SDA_PIN * 2U));
    *BOARD_GPIOB_ODR &= ~(1U << BOARD_I2C_SDA_PIN);
}

static void sda_high(void)
{
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER & ~(3U << (BOARD_I2C_SDA_PIN * 2U)));
}

static uint8_t sda_read(void)
{
    return (uint8_t)((*BOARD_GPIOB_IDR >> BOARD_I2C_SDA_PIN) & 1U);
}

static void i2c_start(void)
{
    sda_high();
    scl_high();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
}

static void i2c_stop(void)
{
    sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

static bool i2c_write_byte(uint8_t byte)
{
    int bit;
    bool ack;

    for (bit = 7; bit >= 0; bit--) {
        if (byte & (1U << (uint8_t)bit)) {
            sda_high();
        } else {
            sda_low();
        }
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
    }
    sda_high(); /* release for ACK */
    i2c_delay();
    scl_high();
    i2c_delay();
    ack = (sda_read() == 0U);
    scl_low();
    return ack;
}

static uint8_t i2c_read_byte(bool nack)
{
    uint8_t byte = 0;
    int bit;

    sda_high();
    for (bit = 7; bit >= 0; bit--) {
        i2c_delay();
        scl_high();
        i2c_delay();
        byte = (uint8_t)((byte << 1) | sda_read());
        scl_low();
    }
    if (nack) {
        sda_high();
    } else {
        sda_low();
    }
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    sda_high();
    return byte;
}

static bool pca_write_reg(uint8_t reg, uint8_t value)
{
    bool ok;
    i2c_start();
    ok = i2c_write_byte((uint8_t)(BOARD_PCA9685_ADDR << 1));
    ok = i2c_write_byte(reg) && ok;
    ok = i2c_write_byte(value) && ok;
    i2c_stop();
    return ok;
}

static bool pca_read_reg(uint8_t reg, uint8_t *value)
{
    bool ok;
    i2c_start();
    ok = i2c_write_byte((uint8_t)(BOARD_PCA9685_ADDR << 1));
    ok = i2c_write_byte(reg) && ok;
    i2c_start();
    ok = i2c_write_byte((uint8_t)((BOARD_PCA9685_ADDR << 1) | 1U)) && ok;
    *value = i2c_read_byte(true);
    i2c_stop();
    return ok;
}

static void pca_set_prescale(uint8_t prescale)
{
    uint8_t mode;
    (void)pca_read_reg(PCA9685_MODE1, &mode);
    (void)pca_write_reg(PCA9685_MODE1, (uint8_t)((mode & 0x7FU) | 0x10U)); /* SLEEP */
    (void)pca_write_reg(PCA9685_PRESCALE, prescale);
    (void)pca_write_reg(PCA9685_MODE1, mode);
    board_delay(5000);
    (void)pca_write_reg(PCA9685_MODE1, (uint8_t)(mode | 0xA1U)); /* AI | ALLCALL | restart */
}

bool pca9685_init(void)
{
    uint8_t mode;
    uint32_t prescale;

    board_gpio_enable_clocks();

    /* PB6/PB7 pull-ups, start released (input) */
    *BOARD_GPIOB_PUPDR = (*BOARD_GPIOB_PUPDR &
                          ~((3U << (BOARD_I2C_SCL_PIN * 2U)) | (3U << (BOARD_I2C_SDA_PIN * 2U)))) |
                         (1U << (BOARD_I2C_SCL_PIN * 2U)) | (1U << (BOARD_I2C_SDA_PIN * 2U));
    *BOARD_GPIOB_MODER = (*BOARD_GPIOB_MODER &
                          ~((3U << (BOARD_I2C_SCL_PIN * 2U)) | (3U << (BOARD_I2C_SDA_PIN * 2U))));
    *BOARD_GPIOB_ODR &= ~((1U << BOARD_I2C_SCL_PIN) | (1U << BOARD_I2C_SDA_PIN));

    board_delay(10000);

    if (!pca_write_reg(PCA9685_MODE1, 0x00U)) {
        return false;
    }
    if (!pca_read_reg(PCA9685_MODE1, &mode)) {
        return false;
    }

    /* prescale = round(osc / (4096 * hz)) - 1 */
    prescale = (PCA9685_OSC_HZ / (4096U * PCA9685_PWM_HZ));
    if (prescale < 3U) {
        prescale = 3U;
    }
    pca_set_prescale((uint8_t)(prescale - 1U));
    pca9685_all_off();
    return true;
}

void pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off)
{
    uint8_t base;

    if (channel > 15U) {
        return;
    }
    base = (uint8_t)(PCA9685_LED0_ON_L + (channel * 4U));
    i2c_start();
    (void)i2c_write_byte((uint8_t)(BOARD_PCA9685_ADDR << 1));
    (void)i2c_write_byte(base);
    (void)i2c_write_byte((uint8_t)(on & 0xFFU));
    (void)i2c_write_byte((uint8_t)((on >> 8) & 0x0FU));
    (void)i2c_write_byte((uint8_t)(off & 0xFFU));
    (void)i2c_write_byte((uint8_t)((off >> 8) & 0x0FU));
    i2c_stop();
}

void pca9685_set_duty(uint8_t channel, uint8_t duty_pct)
{
    uint16_t off;

    if (duty_pct > 100U) {
        duty_pct = 100U;
    }
    if (duty_pct == 0U) {
        pca9685_set_pwm(channel, 0, 0);
        return;
    }
    if (duty_pct >= 100U) {
        pca9685_set_pwm(channel, 0x1000U, 0); /* full on */
        return;
    }
    off = (uint16_t)((4096UL * duty_pct) / 100U);
    pca9685_set_pwm(channel, 0, off);
}

void pca9685_all_off(void)
{
    uint8_t ch;
    for (ch = 0; ch < 16U; ch++) {
        pca9685_set_pwm(ch, 0, 0);
    }
}
