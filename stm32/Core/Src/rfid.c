/*
 * rfid.c - 134 kHz RFID reader on USART2 RX (PA3) @ 9600 8N1
 *
 * Frame: $A0112OKD + 15 decimal digits + 2 hex checksum chars (27 bytes total)
 */
#include "rfid.h"
#include "board.h"
#include <string.h>

#define RFID_FRAME_LEN      27U
#define RFID_HEADER         "$A0112OKD"
#define RFID_BUF_SIZE       256U

static char gs_rfid_buf[RFID_BUF_SIZE];
static uint32_t gs_rfid_count;

static volatile uint32_t *const usart2_cr1  = (volatile uint32_t *)0x40004400U;
static volatile uint32_t *const usart2_brr  = (volatile uint32_t *)0x4000440CU;
static volatile uint32_t *const usart2_isr  = (volatile uint32_t *)0x4000441CU;
static volatile uint32_t *const usart2_rdr  = (volatile uint32_t *)0x40004424U;
static volatile uint32_t *const usart2_icr  = (volatile uint32_t *)0x40004420U;

static char hex_nibble(uint8_t n)
{
    return (char)((n <= 9U) ? (n + 48U) : (n + 55U));
}

static bool parse_frame(const char *frame, uint32_t *tag_out)
{
    char checksum;
    char hi;
    char lo;
    uint64_t tag = 0;
    uint64_t mult = 1;
    int i;

    if (memcmp(frame, RFID_HEADER, 9) != 0) {
        return false;
    }

    checksum = frame[1];
    for (i = 2; i <= 23; i++) {
        checksum = (char)(checksum ^ frame[i]);
    }

    hi = hex_nibble((uint8_t)(((uint8_t)checksum) / 16U));
    lo = hex_nibble((uint8_t)(((uint8_t)checksum) % 16U));
    if (hi != frame[24] || lo != frame[25]) {
        return false;
    }

    for (i = 23; i >= 9; i--) {
        tag += (uint64_t)(frame[i] - '0') * mult;
        mult *= 10U;
    }

    *tag_out = (uint32_t)(tag % 100000000U);
    return true;
}

static void accumulate(void)
{
    uint32_t added = 0;

    while (((*usart2_isr & (1U << 5)) != 0U) && ((gs_rfid_count + added) < RFID_BUF_SIZE)) {
        gs_rfid_buf[gs_rfid_count + added] = (char)(*usart2_rdr & 0xFFU);
        added++;
    }
    gs_rfid_count += added;

    if (gs_rfid_count >= (RFID_BUF_SIZE - 32U)) {
        uint32_t discard = gs_rfid_count / 2U;
        uint32_t keep = gs_rfid_count - discard;
        memmove(gs_rfid_buf, gs_rfid_buf + discard, keep);
        gs_rfid_count = keep;
    }
}

void rfid_init(void)
{
    board_gpio_enable_clocks();
    *BOARD_RCC_APB1ENR1 |= (1U << 17); /* USART2 */

    /* PA3 USART2_RX AF7 */
    *BOARD_GPIOA_MODER = (*BOARD_GPIOA_MODER & ~(3U << 6)) | (2U << 6);
    *BOARD_GPIOA_AFRL = (*BOARD_GPIOA_AFRL & ~(0xFU << 12)) | (7U << 12);

    gs_rfid_count = 0;
    memset(gs_rfid_buf, 0, sizeof(gs_rfid_buf));

    *usart2_cr1 = 0;
    *usart2_brr = BOARD_SYSCLK_HZ / 9600U;
    *usart2_cr1 = (1U << 0) | (1U << 2); /* UE | RE */

    while ((*usart2_isr & (1U << 5)) != 0U) {
        (void)*usart2_rdr;
    }
    *usart2_icr = (1U << 3) | (1U << 2) | (1U << 1) | (1U << 0);
}

bool rfid_poll(uint32_t *tag_out)
{
    uint32_t i;

    if (tag_out == NULL) {
        return false;
    }
    *tag_out = 0;

    accumulate();

    for (i = 0; (i + RFID_FRAME_LEN) <= gs_rfid_count; i++) {
        if (memcmp(gs_rfid_buf + i, RFID_HEADER, 9) == 0) {
            char frame[RFID_FRAME_LEN + 1U];
            uint32_t consumed = i + RFID_FRAME_LEN;

            memcpy(frame, gs_rfid_buf + i, RFID_FRAME_LEN);
            frame[RFID_FRAME_LEN] = '\0';

            if (parse_frame(frame, tag_out)) {
                if (consumed < gs_rfid_count) {
                    memmove(gs_rfid_buf, gs_rfid_buf + consumed, gs_rfid_count - consumed);
                }
                gs_rfid_count -= consumed;
                return true;
            }
        }
    }
    return false;
}
