#include "storage.h"
#include <cstdint>
#include <esp_log.h>
#include <fstream>
#include <random>

static const char *TAG = "data_generator";

void write_random_char_data(const char *file_path, uint32_t num_chars) {
    Storage *storage = Storage::getInstance();

    if (!file_path || num_chars == 0) {
        ESP_LOGE(TAG, "Invalid parameters: file_path=%p, num_chars=%lu", file_path, num_chars);
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (uint32_t i = 0; i < num_chars; ++i) {
        char random_char = static_cast<char>(dis(gen));
        storage->write_file(file_path, &random_char, std::ios::app);
    }

    ESP_LOGI(TAG, "Wrote %lu random characters to %s", num_chars, file_path);
}