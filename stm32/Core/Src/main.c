/*
 * main.c - WiFeeder v2 production firmware entry
 *
 * Superloop calls wifeeder_poll(). FreeRTOS is vendored under
 * Middlewares/Third_Party/FreeRTOS for a later task split.
 * NRF bring-up probe lives in tests/03-nrf/.
 */
#include "board.h"
#include "wifeeder.h"

void SystemInit(void)
{
}

unsigned int SystemCoreClock = BOARD_SYSCLK_HZ;

int main(void)
{
    board_systick_init();
    board_led_init();
    board_led_set(1);

    wifeeder_init();

    for (;;) {
        wifeeder_poll();

        static uint32_t last_blink;
        uint32_t now = board_millis();
        if ((now - last_blink) >= 500U) {
            last_blink = now;
            static int led;
            led = !led;
            board_led_set(led);
        }
    }
}
