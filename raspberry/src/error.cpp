#include "error.h"

#include <cstdlib>

void error_critical(void)
{
    std::abort();
}
