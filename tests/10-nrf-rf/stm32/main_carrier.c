/*
 * Test 10 — continuous RF carrier (Nucleo A)
 *
 * Goal: put energy on the air. No packet protocol.
 *  1) CE pin self-check (GPIOB0 must read high after ce_hi)
 *  2) CONT_WAVE + PLL_LOCK if the clone supports it
 *  3) fallback: forever retransmit dummy payloads (AA off)
 *
 * Probe @ 0x20000000:
 *   [0] 0xA55A0010 OK, 0xDEAD0001 SPI fail, 0xDEAD0002 CE pin stuck low
 *   [1] (CONFIG<<16)|(RF_SETUP<<8)|RF_CH
 *   [2] ce_idr (expect 1)
 *   [3] blast_count
 *
 * Pins: SCK=PA5 MOSI=PA7 MISO=PB1 CSN=PA4 CE=PB0 LED=PB3
 * Adapter VCC = 5V.
 */
void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

volatile unsigned g_magic;
volatile unsigned g_status;
volatile unsigned g_ce;
volatile unsigned g_blasts;

#define RF_CH    76U
#define RF_SETUP 0x07U /* 1 Mbps + PA_MAX + LNA */
#define PAYLOAD  8U

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

static void nrf_write_payload(const unsigned char *buf, unsigned len)
{
    unsigned i;
    csn_lo();
    spi_xfer(0xA0U);
    for (i = 0; i < len; i++) {
        spi_xfer(buf[i]);
    }
    csn_hi();
}

static const unsigned char ADDR[5] = {0xE7U, 0xE7U, 0xE7U, 0xE7U, 0xE7U};

static void gpio_init(void)
{
    RCC_AHB2ENR |= 3U;
    wa(200);

    /* PB3 LED output */
    GPIOB_MODER = (GPIOB_MODER & ~(3U << 6)) | (1U << 6);
    GPIOB_BSRR = (1U << 3) << 16;
    /* PA4 CSN, PA5 SCK, PA7 MOSI outputs */
    GPIOA_MODER = (GPIOA_MODER & ~((3U << 8) | (3U << 10) | (3U << 14))) |
                  (1U << 8) | (1U << 10) | (1U << 14);
    /* PB0 CE output; PB1 MISO input + pull-up */
    GPIOB_MODER = (GPIOB_MODER & ~((3U << 0) | (3U << 2))) | (1U << 0);
    GPIOB_PUPDR = (GPIOB_PUPDR & ~(3U << 2)) | (1U << 2);

    csn_hi();
    sck_lo();
    ce_lo();
}

static int nrf_init(void)
{
    unsigned char st, ch, setup;

    ce_lo();
    csn_hi();
    delay_ms(200);

    nrf_cmd(0xE1U);
    nrf_cmd(0xE2U);
    nrf_write_reg(0x07, 0x70U);

    csn_lo();
    spi_xfer(0x50U); /* ACTIVATE */
    spi_xfer(0x73U);
    csn_hi();
    nrf_write_reg(0x1DU, 0x00U);
    nrf_write_reg(0x1CU, 0x00U);

    nrf_write_reg(0x00, 0x0CU); /* PWR_UP, CRC, TX */
    nrf_write_reg(0x01, 0x00U); /* no AA */
    nrf_write_reg(0x02, 0x01U);
    nrf_write_reg(0x03, 0x03U);
    nrf_write_reg(0x04, 0x00U); /* ARC=0 */
    nrf_write_reg(0x05, RF_CH);
    nrf_write_reg(0x06, RF_SETUP);
    nrf_write_reg(0x11, PAYLOAD);
    nrf_write_buf(0x0AU, ADDR, 5);
    nrf_write_buf(0x10U, ADDR, 5);

    st = nrf_read_reg(0x07);
    ch = nrf_read_reg(0x05);
    setup = nrf_read_reg(0x06);
    g_status = ((unsigned)nrf_read_reg(0x00) << 16) | ((unsigned)setup << 8) | ch;

    if (st == 0xFFU || ch != RF_CH) {
        return -1;
    }
    return 0;
}

int main(void)
{
    unsigned char pkt[PAYLOAD];
    unsigned i;
    unsigned char st;

    *(volatile unsigned *)0x20000000U = 0x11111111U;
    *(volatile unsigned *)0x20000004U = 0;
    *(volatile unsigned *)0x20000008U = 0;
    *(volatile unsigned *)0x2000000CU = 0;

    gpio_init();

    /* boot: 1 long blink = carrier */
    GPIOB_BSRR = (1U << 3);
    delay_ms(400);
    GPIOB_BSRR = (1U << 3) << 16;
    delay_ms(200);

    /* CE self-check before radio */
    ce_hi();
    delay_ms(1);
    g_ce = (GPIOB_IDR & 1U);
    *(volatile unsigned *)0x20000008U = g_ce;
    if (g_ce == 0U) {
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

    /* Try continuous carrier (Nordic CONT_WAVE|PLL_LOCK) */
    nrf_write_reg(0x00, 0x0EU);
    delay_ms(5);
    nrf_write_reg(0x06, (unsigned char)(0x90U | RF_SETUP)); /* CONT_WAVE|PLL_LOCK|PA */
    ce_hi();
    delay_ms(5);

    g_status = ((unsigned)nrf_read_reg(0x00) << 16) |
               ((unsigned)nrf_read_reg(0x06) << 8) | nrf_read_reg(0x05);
    g_ce = (GPIOB_IDR & 1U);
    g_magic = 0xA55A0010U;
    *(volatile unsigned *)0x20000000U = g_magic;
    *(volatile unsigned *)0x20000004U = g_status;
    *(volatile unsigned *)0x20000008U = g_ce;

    for (i = 0; i < PAYLOAD; i++) {
        pkt[i] = (unsigned char)(0xA0U + i);
    }

    /* Forever: keep blasts going (covers clones that ignore CONT_WAVE) */
    for (;;) {
        nrf_write_reg(0x06, (unsigned char)(0x90U | RF_SETUP));
        nrf_write_reg(0x00, 0x0EU);
        nrf_cmd(0xE1U);
        nrf_write_reg(0x07, 0x70U);
        nrf_write_payload(pkt, PAYLOAD);
        ce_hi();
        delay_ms(3);
        st = nrf_read_reg(0x07);
        (void)st;
        /* leave CE high most of the time for CONT_WAVE */
        delay_ms(10);
        g_blasts++;
        g_ce = (GPIOB_IDR & 1U);
        g_status = ((unsigned)nrf_read_reg(0x00) << 16) |
                   ((unsigned)nrf_read_reg(0x06) << 8) | nrf_read_reg(0x05);
        *(volatile unsigned *)0x20000000U = g_magic;
        *(volatile unsigned *)0x20000004U = g_status;
        *(volatile unsigned *)0x20000008U = g_ce;
        *(volatile unsigned *)0x2000000CU = g_blasts;
        GPIOB_ODR ^= (1U << 3);
    }
}
