void SystemInit(void) {}
unsigned int SystemCoreClock = 4000000;

#include "../common/test_utils.h"

int main(void)
{
    while (1) {
        test_blink_pattern(1U, 2000U, 2000U);
    }
}
