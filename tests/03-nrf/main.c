/*
 * Test 03 — NRF24L01+ SPI probe (slow bit-bang)
 * Board labels: SCK=A4 MOSI=A6 MISO=D6 CSN=A3 CE=D3 VCC=3.3V
 * Results @ 0x20000000..0x2000000C for OpenOCD
 */
void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

volatile unsigned g_nrf_magic, g_nrf_nop, g_nrf_status, g_nrf_rfch, g_nrf_result;

static void wa(unsigned n)
{
    for (volatile unsigned i = 0; i < n; i++) {
    }
}

static void delay_ms(unsigned ms)
{
    while (ms--) {
        wa(1000);
    }
}

#define RCC_AHB2ENR (*(volatile unsigned *)0x4002104C)
#define GPIOA_MODER (*(volatile unsigned *)0x48000000)
#define GPIOA_ODR   (*(volatile unsigned *)0x48000014)
#define GPIOB_MODER (*(volatile unsigned *)0x48000400)
#define GPIOB_ODR   (*(volatile unsigned *)0x48000414)
#define GPIOB_IDR   (*(volatile unsigned *)0x48000410)
#define GPIOB_PUPDR (*(volatile unsigned *)0x4800040C)

static void sck_hi(void) { GPIOA_ODR |= (1U << 5); wa(30); }
static void sck_lo(void) { GPIOA_ODR &= ~(1U << 5); wa(30); }
static void csn_lo(void) { GPIOA_ODR &= ~(1U << 4); wa(80); }
static void csn_hi(void) { GPIOA_ODR |= (1U << 4); wa(80); }

static unsigned char spi_xfer(unsigned char out)
{
    unsigned char in = 0;
    int b;
    for (b = 7; b >= 0; b--) {
        if (out & (1U << (unsigned)b)) {
            GPIOA_ODR |= (1U << 7);
        } else {
            GPIOA_ODR &= ~(1U << 7);
        }
        wa(20);
        sck_hi();
        in = (unsigned char)((in << 1) | ((GPIOB_IDR >> 1) & 1U));
        sck_lo();
    }
    return in;
}

static unsigned char nrf_read_reg(unsigned char reg)
{
    unsigned char v;
    csn_lo();
    spi_xfer(reg & 0x1FU);
    v = spi_xfer(0xFF);
    csn_hi();
    return v;
}

static void nrf_write_reg(unsigned char reg, unsigned char val)
{
    csn_lo();
    spi_xfer((unsigned char)(0x20U | (reg & 0x1FU)));
    spi_xfer(val);
    csn_hi();
}

int main(void)
{
    unsigned char s1, s3, s4;
    unsigned stat;
    unsigned miso_idle;

    *(volatile unsigned *)0x20000000U = 0x11111111U;

    RCC_AHB2ENR |= 3U;
    wa(200);

    GPIOB_MODER = (GPIOB_MODER & ~(3U << 6)) | (1U << 6);
    GPIOB_ODR &= ~(1U << 3);

    /* PA4 CSN, PA5 SCK, PA7 MOSI out; PB0 CE out; PB1 MISO in + pull-up */
    GPIOA_MODER = (GPIOA_MODER & ~((3U << 8) | (3U << 10) | (3U << 14))) |
                  (1U << 8) | (1U << 10) | (1U << 14);
    GPIOB_MODER = (GPIOB_MODER & ~((3U << 0) | (3U << 2))) | (1U << 0);
    GPIOB_PUPDR = (GPIOB_PUPDR & ~(3U << 2)) | (1U << 2);

    GPIOA_ODR |= (1U << 4);
    GPIOA_ODR &= ~(1U << 5);
    GPIOB_ODR &= ~(1U << 0); /* CE low for register access */

    delay_ms(100); /* power-on settle */

    for (int i = 0; i < 3; i++) {
        GPIOB_ODR |= (1U << 3);
        delay_ms(80);
        GPIOB_ODR &= ~(1U << 3);
        delay_ms(80);
    }

    miso_idle = (GPIOB_IDR >> 1) & 1U;

    csn_lo();
    s1 = spi_xfer(0xFF); /* NOP → STATUS */
    csn_hi();

    s3 = nrf_read_reg(0x07); /* STATUS */
    nrf_write_reg(0x05, 0x4C); /* RF_CH = 76 */
    s4 = nrf_read_reg(0x05);

    stat = (s3 != 0xFF) ? s3 : s1;

    g_nrf_nop = s1;
    g_nrf_status = s3;
    g_nrf_rfch = s4;
    if (stat == 0x0E && s4 == 0x4C) {
        g_nrf_result = 1;
    } else if (stat == 0x0E) {
        g_nrf_result = 2;
    } else if (stat == 0xFF || s1 == 0xFF) {
        g_nrf_result = 3;
    } else {
        g_nrf_result = 4;
    }
    g_nrf_magic = 0xA55A0001U;

    *(volatile unsigned *)0x20000000U = 0xA55A0000U | g_nrf_result;
    *(volatile unsigned *)0x20000004U = g_nrf_nop | (miso_idle << 8);
    *(volatile unsigned *)0x20000008U = g_nrf_status;
    *(volatile unsigned *)0x2000000CU = g_nrf_rfch;

    if (g_nrf_result == 1) {
        for (;;) {
            GPIOB_ODR |= (1U << 3);
            delay_ms(5000);
            GPIOB_ODR &= ~(1U << 3);
            delay_ms(1000);
        }
    } else if (g_nrf_result == 2) {
        for (;;) {
            GPIOB_ODR |= (1U << 3);
            delay_ms(3000);
            GPIOB_ODR &= ~(1U << 3);
            delay_ms(1000);
        }
    } else if (g_nrf_result == 3) {
        for (;;) {
            GPIOB_ODR |= (1U << 3);
            delay_ms(2000);
            GPIOB_ODR &= ~(1U << 3);
            delay_ms(2000);
        }
    } else {
        for (;;) {
            GPIOB_ODR |= (1U << 3);
            delay_ms(400);
            GPIOB_ODR &= ~(1U << 3);
            delay_ms(400);
        }
    }
}
