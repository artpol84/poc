#include <stdio.h>

volatile int start_imitate = 0;

__attribute__((weak)) int debug_add_credits(int count) {
    start_imitate += count;
    return 0;
}

