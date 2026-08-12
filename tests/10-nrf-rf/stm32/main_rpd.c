/*
 * Test 10 — RF energy detector (Nucleo B)
 *
 * Stay in RX with CE high; sample RPD (reg 0x09) and RX_DR.
 * Does not require a valid address match for RPD — any strong
 * 2.4 GHz energy on-channel sets RPD.
 *
 * Probe @ 0x20000000:
 *   [0] 0xA55A0011 OK, 0xDEAD0001 SPI fail, 0xDEAD0002 CE stuck low
 *   [1] (CONFIG<<16)|(RF_SETUP<<8)|RF_CH
 *   [2] samples
 *   [3] rpd_hits | (rx_dr<<16)   — PASS if rpd_hits > 0
 *
 * After ~3 s: magic → 0xA55A00FF if any RPD, else 0xA55A00F0
 *
 * Pins: SCK=PA5 MOSI=PA7 MISO=PB1 CSN=PA4 CE=PB0 LED=PB3
 * Adapter VCC = 5V.
 */
void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

volatile unsigned g_magic;
volatile unsigned g_status;
volatile unsigned g_samples;
volatile unsigned g_hits;

#define RF_CH    76U
#define RF_SETUP 0x07U
#define PAYLOAD  8U
#define CONFIG_RX 0x0FU
#define SAMPLE_MS 4000U

static void wa(unsigned n)
{
    for (volatile unsigned i = 0; i < n; i++) {
    }
}

static void delay_ms(unsigned ms)
{
    while (ms--) {
        wa(400);
    }
}

#define RCC_AHB2ENR (*(volatile unsigned *)0x4002104C)
#define GPIOA_MODER (*(volatile unsigned *)0x48000000)
#define GPIOA_BSRR  (*(volatile unsigned *)0x48000018)
#define GPIOB_MODER (*(volatile unsigned *)0x48000400)
#define GPIOB_ODR   (*(volatile unsigned *)0x48000414)
#define GPIOB_IDR   (*(volatile unsigned *)0x48000410)
#define GPIOB_BSRR  (*(volatile unsigned *)0x48000418)
#define GPIOB_PUPDR (*(volatile unsigned *)0x4800040C)

static void sck_hi(void) { GPIOA_BSRR = (1U << 5); wa(40); }
static void sck_lo(void) { GPIOA_BSRR = (1U << 5) << 16; wa(40); }
static void csn_lo(void) { GPIOA_BSRR = (1U << 4) << 16; wa(100); }
static void csn_hi(void) { GPIOA_BSRR = (1U << 4); wa(100); }
static void ce_lo(void)  { GPIOB_BSRR = (1U << 0) << 16; }
static void ce_hi(void)  { GPIOB_BSRR = (1U << 0); }
static void mosi_set(int v)
{
    if (v) {
        GPIOA_BSRR = (1U << 7);
    } else {
        GPIOA_BSRR = (1U << 7) << 16;
    }
}

static unsigned char spi_xfer(unsigned char out)
{
    unsigned char in = 0;
    int b;
    for (b = 7; b >= 0; b--) {
        mosi_set(out & (1U << (unsigned)b));
        wa(30);
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

static void nrf_write_buf(unsigned char reg, const unsigned char *buf, unsigned len)
{
    unsigned i;
    csn_lo();
    spi_xfer((unsigned char)(0x20U | (reg & 0x1FU)));
    for (i = 0; i < len; i++) {
        spi_xfer(buf[i]);
    }
    csn_hi();
}

static void nrf_cmd(unsigned char cmd)
{
    csn_lo();
    spi_xfer(cmd);
    csn_hi();
}

static const unsigned char ADDR[5] = {0xE7U, 0xE7U, 0xE7U, 0xE7U, 0xE7U};

static void gpio_init(void)
{
    RCC_AHB2ENR |= 3U;
    wa(200);

    GPIOB_MODER = (GPIOB_MODER & ~(3U << 6)) | (1U << 6);
    GPIOB_BSRR = (1U << 3) << 16;
    GPIOA_MODER = (GPIOA_MODER & ~((3U << 8) | (3U << 10) | (3U << 14))) |
                  (1U << 8) | (1U << 10) | (1U << 14);
    GPIOB_MODER = (GPIOB_MODER & ~((3U << 0) | (3U << 2))) | (1U << 0);
    GPIOB_PUPDR = (GPIOB_PUPDR & ~(3U << 2)) | (1U << 2);

    csn_hi();
    sck_lo();
    ce_lo();
}

static int nrf_init(void)
{
    unsigned char st, ch, cfg, setup;

    ce_lo();
    csn_hi();
    delay_ms(200);

    nrf_cmd(0xE1U);
    nrf_cmd(0xE2U);
    nrf_write_reg(0x07, 0x70U);

    csn_lo();
    spi_xfer(0x50U);
    spi_xfer(0x73U);
    csn_hi();
    nrf_write_reg(0x1DU, 0x00U);
    nrf_write_reg(0x1CU, 0x00U);

    nrf_write_reg(0x00, 0x0CU);
    nrf_write_reg(0x01, 0x00U);
    nrf_write_reg(0x02, 0x01U);
    nrf_write_reg(0x03, 0x03U);
    nrf_write_reg(0x04, 0x00U);
    nrf_write_reg(0x05, RF_CH);
    nrf_write_reg(0x06, RF_SETUP);
    nrf_write_reg(0x11, PAYLOAD);
    nrf_write_buf(0x0AU, ADDR, 5);
    nrf_write_buf(0x10U, ADDR, 5);

    /* Enter RX with CE high */
    nrf_write_reg(0x00, CONFIG_RX);
    delay_ms(5);
    ce_hi();
    delay_ms(5);

    st = nrf_read_reg(0x07);
    ch = nrf_read_reg(0x05);
    setup = nrf_read_reg(0x06);
    cfg = nrf_read_reg(0x00);
    g_status = ((unsigned)cfg << 16) | ((unsigned)setup << 8) | ch;

    if (st == 0xFFU || ch != RF_CH || cfg != CONFIG_RX) {
        return -1;
    }
    return 0;
}

int main(void)
{
    unsigned rpd_hits = 0;
    unsigned rx_dr = 0;
    unsigned t;
    unsigned char rpd, st, ce;

    *(volatile unsigned *)0x20000000U = 0x11111111U;
    *(volatile unsigned *)0x20000004U = 0;
    *(volatile unsigned *)0x20000008U = 0;
    *(volatile unsigned *)0x2000000CU = 0;

    gpio_init();

    /* boot: 3 short blinks = RPD listener */
    for (t = 0; t < 3U; t++) {
        GPIOB_BSRR = (1U << 3);
        delay_ms(80);
        GPIOB_BSRR = (1U << 3) << 16;
        delay_ms(80);
    }

    ce_hi();
    delay_ms(1);
    ce = (unsigned char)(GPIOB_IDR & 1U);
    *(volatile unsigned *)0x20000008U = ce;
    if (ce == 0U) {
        g_magic = 0xDEAD0002U;
        *(volatile unsigned *)0x20000000U = g_magic;
        for (;;) {
            GPIOB_ODR ^= (1U << 3);
            delay_ms(80);
        }
    }
    ce_lo();

    if (nrf_init() != 0) {
        g_magic = 0xDEAD0001U;
        *(volatile unsigned *)0x20000000U = g_magic;
        *(volatile unsigned *)0x20000004U = g_status;
        for (;;) {
            GPIOB_ODR ^= (1U << 3);
            delay_ms(100);
        }
    }

    g_magic = 0xA55A0011U;
    *(volatile unsigned *)0x20000000U = g_magic;
    *(volatile unsigned *)0x20000004U = g_status;

    for (t = 0; t < SAMPLE_MS; t++) {
        rpd = nrf_read_reg(0x09);
        st = nrf_read_reg(0x07);
        if (rpd & 0x01U) {
            rpd_hits++;
        }
        if (st & 0x40U) {
            rx_dr++;
            nrf_write_reg(0x07, 0x70U);
            nrf_cmd(0xE2U);
        }
        g_samples = t + 1U;
        g_hits = rpd_hits | (rx_dr << 16);
        *(volatile unsigned *)0x20000008U = g_samples;
        *(volatile unsigned *)0x2000000CU = g_hits;
        if ((t & 0x7FU) == 0U) {
            GPIOB_ODR ^= (1U << 3);
        }
        delay_ms(1);
    }

    if (rpd_hits > 0U) {
        g_magic = 0xA55A00FFU; /* energy seen */
        GPIOB_BSRR = (1U << 3); /* solid LED */
    } else {
        g_magic = 0xA55A00F0U; /* no energy */
    }
    *(volatile unsigned *)0x20000000U = g_magic;
    *(volatile unsigned *)0x20000008U = g_samples;
    *(volatile unsigned *)0x2000000CU = g_hits;

    for (;;) {
        if (rpd_hits > 0U) {
            delay_ms(500);
        } else {
            GPIOB_ODR ^= (1U << 3);
            delay_ms(100);
        }
    }
}
