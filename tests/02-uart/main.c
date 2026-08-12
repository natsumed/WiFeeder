/* USART1 TX on PA9 @ 115200, HSI 16 MHz — register level */
void SystemInit(void) {}
unsigned int SystemCoreClock = 16000000;

#include "../common/test_utils.h"
#include <stdio.h>

#define RCC_CR        (*(volatile uint32_t *)0x40021000)
#define RCC_CFGR      (*(volatile uint32_t *)0x40021008)
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40021060)
#define RCC_AHB2ENR   (*(volatile uint32_t *)0x4002104C)

#define GPIOA_MODER   (*(volatile uint32_t *)0x48000000)
#define GPIOA_AFRL    (*(volatile uint32_t *)0x48000020)
#define GPIOA_AFRH    (*(volatile uint32_t *)0x48000024)

#define USART1_CR1    (*(volatile uint32_t *)0x40013800)
#define USART1_BRR    (*(volatile uint32_t *)0x4001380C)
#define USART1_ISR    (*(volatile uint32_t *)0x4001381C)
#define USART1_TDR    (*(volatile uint32_t *)0x40013828)

#define RCC_CR_HSION  (1U << 0)
#define RCC_CR_HSIRDY (1U << 2)
#define RCC_CFGR_SW_HSI (0U << 0)
#define RCC_CFGR_SWS_HSI (0U << 2)

static void clock_hsi_16mhz(void)
{
    RCC_CR |= RCC_CR_HSION;
    while ((RCC_CR & RCC_CR_HSIRDY) == 0U) {
    }
    RCC_CFGR = (RCC_CFGR & ~3U) | RCC_CFGR_SW_HSI;
    while ((RCC_CFGR & RCC_CFGR_SWS_HSI) != RCC_CFGR_SWS_HSI) {
    }
}

static void usart1_init(void)
{
    RCC_AHB2ENR |= (1U << 0);
    RCC_APB2ENR |= (1U << 14);
    for (volatile uint32_t i = 0; i < 100U; i++) {
    }

    /* PA9 = USART1_TX AF7 */
    GPIOA_MODER = (GPIOA_MODER & ~(3U << 18)) | (2U << 18);
    GPIOA_AFRH = (GPIOA_AFRH & ~(0xFU << 4)) | (7U << 4);

    USART1_CR1 = 0;
    USART1_BRR = 139U; /* 16 MHz / 115200 ≈ 139 */
    USART1_CR1 = (1U << 0) | (1U << 3); /* UE | TE */
}

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int _write(int fd, const char *ptr, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) {
        while ((USART1_ISR & (1U << 7)) == 0U) {
        }
        USART1_TDR = (uint8_t)ptr[i];
    }
    return len;
}

void _exit(int status) { (void)status; while (1) { } }
int _close(int fd) { (void)fd; return -1; }
int _fstat(int fd, struct stat *st) { (void)fd; (void)st; return 0; }
int _isatty(int fd) { (void)fd; return 1; }
int _lseek(int fd, int ptr, int dir) { (void)fd; (void)ptr; (void)dir; return 0; }
int _read(int fd, char *ptr, int len) { (void)fd; (void)ptr; (void)len; return 0; }
void *_sbrk(int incr)
{
    extern char _end;
    static char *heap = 0;
    if (!heap) {
        heap = &_end;
    }
    char *prev = heap;
    heap += incr;
    return prev;
}
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
void _init(void) {}

int main(void)
{
    clock_hsi_16mhz();
    usart1_init();
    test_led_init();

    while (1) {
        puts("=== NUCLEO-L432KC UART OK ===");
        test_blink_pattern(1U, 100U, 100U);
        test_delay_ms(5000U);
    }
}
