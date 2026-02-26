#include <stdio.h>
#include "system_info.h"
#include "esp_psram.h"
#include "esp_private/esp_clk.h"

size_t getPsramSize(void)
{
    return esp_psram_get_size();
}

uint32_t getCpuFreqMHz(void)
{
    return esp_clk_cpu_freq() / 1000000;
}