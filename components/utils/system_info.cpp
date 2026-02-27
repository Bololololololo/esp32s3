#include "system_info.h"

#include "system_info.h"
#include "esp_psram.h"
#include "esp_private/esp_clk.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "sys_info";

namespace utils
{
    using namespace std;

    SystemInfo::SystemInfo()
    {
        ESP_LOGI(TAG, "Initializing system info");
    }

    SystemInfo::~SystemInfo()
    {
        ESP_LOGI(TAG, "Deinitializing system info");
    }

    size_t SystemInfo::getPsramSize(void)
    {
        return esp_psram_get_size() / 1024;
    }

    uint32_t SystemInfo::getCpuFreqMHz(void)
    {
        return esp_clk_cpu_freq() / 1000000;
    }

    size_t SystemInfo::getFreeHeapSizeKB(void)
    {
        // return esp_get_free_heap_size() / 1024;
        return 1024;
    }

    size_t SystemInfo::getTotalHeapSizeKB(void)
    {
        // return esp_get_total_heap_size() / 1024;
        return 1024;
    }
}