/* 134 kHz RFID reader on USART2 RX PA3 @ 9600 — $A0112OKD frame parse (v1 algorithm) */
void SystemInit(void) {}
unsigned int SystemCoreClock = 16000000;

#include "../common/test_utils.h"
#include <string.h>

#define RCC_CR        (*(volatile uint32_t *)0x40021000)
#define RCC_CFGR      (*(volatile uint32_t *)0x40021008)
#define RCC_APB1ENR1  (*(volatile uint32_t *)0x40021058)
#define RCC_AHB2ENR   (*(volatile uint32_t *)0x4002104C)

#define GPIOA_MODER   (*(volatile uint32_t *)0x48000000)
#define GPIOA_AFRL    (*(volatile uint32_t *)0x48000020)

#define USART2_CR1    (*(volatile uint32_t *)0x40004400)
#define USART2_BRR    (*(volatile uint32_t *)0x4000440C)
#define USART2_ISR    (*(volatile uint32_t *)0x4000441C)
#define USART2_RDR    (*(volatile uint32_t *)0x40004424)

#define RFID_BUF_SIZE 256U

static char s_rx_buf[RFID_BUF_SIZE];
static uint32_t s_rx_count;

static void clock_hsi_16mhz(void)
{
    RCC_CR |= (1U << 0);
    while ((RCC_CR & (1U << 2)) == 0U) {
    }
    RCC_CFGR = (RCC_CFGR & ~3U);
    while ((RCC_CFGR & (3U << 2)) != 0U) {
    }
}

static void usart2_init(void)
{
    RCC_AHB2ENR |= (1U << 0);
    RCC_APB1ENR1 |= (1U << 17);
    for (volatile uint32_t i = 0; i < 100U; i++) {
    }

    /* PA3 = USART2_RX AF7 */
    GPIOA_MODER = (GPIOA_MODER & ~(3U << 6)) | (2U << 6);
    GPIOA_AFRL = (GPIOA_AFRL & ~(0xFU << 12)) | (7U << 12);

    USART2_CR1 = 0;
    USART2_BRR = 1667U;
    USART2_CR1 = (1U << 0) | (1U << 2) | (1U << 5);
}

static void rfid_accumulate(void)
{
    while ((USART2_ISR & (1U << 5)) != 0U && s_rx_count < RFID_BUF_SIZE) {
        s_rx_buf[s_rx_count++] = (char)(USART2_RDR & 0xFFU);
    }
    if (s_rx_count >= RFID_BUF_SIZE - 32U) {
        uint32_t discard = s_rx_count / 2U;
        memmove(s_rx_buf, s_rx_buf + discard, s_rx_count - discard);
        s_rx_count -= discard;
    }
}

static char hex_nibble(char v)
{
    return (char)((v <= 9) ? (v + '0') : (v + '7'));
}

static int rfid_check_frame(const char *frame, uint32_t *tag_id)
{
    char checksum;
    char hi;
    char lo;
    uint64_t id = 0U;
    uint64_t mult = 1U;

    *tag_id = 0U;
    if (memcmp(frame, "$A0112OKD", 9) != 0) {
        return 0;
    }

    checksum = frame[1];
    for (int i = 2; i <= 23; i++) {
        checksum = (char)(checksum ^ frame[i]);
    }

    hi = hex_nibble((char)(checksum / 16));
    if (hi != frame[24]) {
        return 0;
    }
    lo = hex_nibble((char)(checksum % 16));
    if (lo != frame[25]) {
        return 0;
    }

    for (int i = 23; i >= 9; i--) {
        id += (uint64_t)(frame[i] - '0') * mult;
        mult *= 10U;
    }
    *tag_id = (uint32_t)(id % 100000000U);
    return 1;
}

static void rfid_blink_tag_id(uint32_t tag_id)
{
    for (int i = 31; i >= 0; i--) {
        if (tag_id & (1U << (uint32_t)i)) {
            test_blink_pattern(1U, 80U, 80U);
        }
        test_delay_ms(30U);
    }
    test_blink_pattern(3U, 500U, 500U);
}

int main(void)
{
    clock_hsi_16mhz();
    usart2_init();
    test_led_init();

    while (1) {
        rfid_accumulate();
        for (uint32_t i = 0; i + 27U <= s_rx_count; i++) {
            if (memcmp(s_rx_buf + i, "$A0112OKD", 9) == 0) {
                char frame[28];
                uint32_t tag_id;
                memcpy(frame, s_rx_buf + i, 27U);
                frame[27] = '\0';
                if (rfid_check_frame(frame, &tag_id)) {
                    rfid_blink_tag_id(tag_id);
                }
                uint32_t consumed = i + 27U;
                if (consumed < s_rx_count) {
                    memmove(s_rx_buf, s_rx_buf + consumed, s_rx_count - consumed);
                }
                s_rx_count -= consumed;
                break;
            }
        }
        test_delay_ms(5U);
    }
}
