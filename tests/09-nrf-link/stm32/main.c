/*
 * Test 09 — STM32 NRF RX peer (no Auto-ACK — simpler on clones/PA+LNA)
 * Pins: SCK=PA5 MOSI=PA7 MISO=PB1 CSN=PA4 CE=PB0 LED=PB3
 * Adapter VCC must be 5V (AM1117).
 */
void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

volatile unsigned g_link_magic;
volatile unsigned g_link_pings;
volatile unsigned g_link_pongs;
volatile unsigned g_link_status; /* live CONFIG<<16 | SETUP<<8 | CH */
volatile unsigned g_link_loops;
volatile unsigned g_link_rxany;
volatile unsigned g_link_rpd;

#define RF_CH       76U
/* 1 Mbps + PA_MAX + LNA enable (RF24 setPALevel(MAX, true) → low 3 bits = 0b111) */
#define RF_SETUP    0x07U
#define PAYLOAD_LEN 8U
#define CONFIG_RX   0x0FU
#define CONFIG_TX   0x0EU

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

static void nrf_read_payload(unsigned char *buf, unsigned len)
{
    unsigned i;
    csn_lo();
    spi_xfer(0x61U);
    for (i = 0; i < len; i++) {
        buf[i] = spi_xfer(0xFF);
    }
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

static void nrf_set_rx(void)
{
    ce_lo();
    nrf_write_reg(0x00, CONFIG_RX);
    delay_ms(5);
    ce_hi();
    delay_ms(5);
}

static void nrf_set_tx(void)
{
    ce_lo();
    nrf_write_reg(0x00, CONFIG_TX);
    delay_ms(5);
}

static int nrf_init_radio(void)
{
    unsigned char st, ch, cfg, setup, aa, pw;

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

    /* No AA — avoids ACK power spikes on PA+LNA */
    nrf_write_reg(0x00, 0x0CU);
    nrf_write_reg(0x01, 0x00U);
    nrf_write_reg(0x02, 0x01U);
    nrf_write_reg(0x03, 0x03U);
    nrf_write_reg(0x04, 0x00U);
    nrf_write_reg(0x05, RF_CH);
    nrf_write_reg(0x06, RF_SETUP);
    nrf_write_reg(0x11, PAYLOAD_LEN);
    nrf_write_buf(0x0AU, ADDR, 5);
    nrf_write_buf(0x10U, ADDR, 5);

    delay_ms(5);
    nrf_set_rx();

    st = nrf_read_reg(0x07);
    ch = nrf_read_reg(0x05);
    setup = nrf_read_reg(0x06);
    cfg = nrf_read_reg(0x00);
    aa = nrf_read_reg(0x01);
    pw = nrf_read_reg(0x11);
    g_link_status = ((unsigned)cfg << 16) | ((unsigned)setup << 8) | ch;

    if (st == 0xFFU || ch != RF_CH || cfg != CONFIG_RX || pw != PAYLOAD_LEN || aa != 0U) {
        return -1;
    }
    return 0;
}

static int payload_is_ping(const unsigned char *buf)
{
    return (buf[0] == 'P' && buf[1] == 'I' && buf[2] == 'N' && buf[3] == 'G');
}

static void send_pong(unsigned char seq)
{
    unsigned char pkt[PAYLOAD_LEN];
    unsigned i;

    for (i = 0; i < PAYLOAD_LEN; i++) {
        pkt[i] = 0;
    }
    pkt[0] = 'P';
    pkt[1] = 'O';
    pkt[2] = 'N';
    pkt[3] = 'G';
    pkt[4] = seq;

    nrf_set_tx();
    nrf_cmd(0xE1U);
    nrf_write_reg(0x07, 0x70U);
    nrf_write_payload(pkt, PAYLOAD_LEN);
    ce_hi();
    delay_ms(2); /* no-AA: short CE pulse is enough */
    ce_lo();
    delay_ms(1);
    nrf_write_reg(0x07, 0x70U);
    nrf_set_rx();
}

int main(void)
{
    unsigned char rx[PAYLOAD_LEN];
    unsigned char st, fifo, cfg;

    *(volatile unsigned *)0x20000000U = 0x11111111U;
    *(volatile unsigned *)0x20000004U = 0;
    *(volatile unsigned *)0x20000008U = 0;
    *(volatile unsigned *)0x2000000CU = 0;

    g_link_magic = 0;
    g_link_pings = 0;
    g_link_pongs = 0;
    g_link_loops = 0;
    g_link_rxany = 0;
    g_link_rpd = 0;

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

    for (int i = 0; i < 3; i++) {
        GPIOB_BSRR = (1U << 3);
        delay_ms(40);
        GPIOB_BSRR = (1U << 3) << 16;
        delay_ms(40);
    }

    if (nrf_init_radio() != 0) {
        g_link_magic = 0xDEAD0001U;
        *(volatile unsigned *)0x20000000U = g_link_magic;
        *(volatile unsigned *)0x20000004U = g_link_status;
        for (;;) {
            GPIOB_ODR ^= (1U << 3);
            delay_ms(150);
        }
    }

    g_link_magic = 0xA55A0009U;
    *(volatile unsigned *)0x20000000U = g_link_magic;
    *(volatile unsigned *)0x20000004U = g_link_status;

    /* Fast RX poll — Nucleo↔Nucleo (no Pi). LED blinks on each PING. */
    for (;;) {
        g_link_loops++;
        g_link_rpd |= (unsigned)(nrf_read_reg(0x09U) & 1U);
        cfg = nrf_read_reg(0x00);
        st = nrf_read_reg(0x07);
        fifo = nrf_read_reg(0x17);
        g_link_status = ((unsigned)cfg << 16) | ((unsigned)nrf_read_reg(0x06) << 8) | nrf_read_reg(0x05);
        *(volatile unsigned *)0x20000004U = g_link_status;
        *(volatile unsigned *)0x20000008U = g_link_loops;
        *(volatile unsigned *)0x2000000CU =
            g_link_pings | (g_link_pongs << 8) | (g_link_rxany << 16) | (g_link_rpd << 24);

        if ((st & 0x40U) || ((fifo & 0x01U) == 0U)) {
            nrf_read_payload(rx, PAYLOAD_LEN);
            nrf_write_reg(0x07, 0x70U);
            nrf_cmd(0xE2U);
            g_link_rxany++;
            if (payload_is_ping(rx)) {
                g_link_pings++;
                send_pong(rx[4]);
                g_link_pongs++;
                GPIOB_BSRR = (1U << 3);
                delay_ms(20);
                GPIOB_BSRR = (1U << 3) << 16;
            }
        }
    }
}
