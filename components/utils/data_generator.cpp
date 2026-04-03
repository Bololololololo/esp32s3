#include "storage.h"
#include <cstdint>
#include <esp_log.h>
#include <fstream>
#include <random>

static const char *TAG = "data_generator";

#define BUFFER_SIZE 100

void write_random_char_data(const char *file_path, uint32_t num_chars) {
    Storage *storage = Storage::getInstance();
    char buffer[BUFFER_SIZE];

    esp_err_t ret{ESP_OK};

    if (!file_path || num_chars == 0) {
        ESP_LOGE(TAG, "Invalid parameters: file_path=%p, num_chars=%lu", file_path, num_chars);
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (uint32_t i = 0; i < num_chars && ret == ESP_OK; ++i) {
        char random_char = static_cast<char>(dis(gen));
        buffer[i] = random_char;
        if (i % BUFFER_SIZE == BUFFER_SIZE - 1 || i == num_chars - 1) {
            ret = storage->write_file(file_path, buffer, std::ios::app);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to write to file. Error: %s", esp_err_to_name(ret));
                return;
            }
        }
    }

    ESP_LOGI(TAG, "Wrote %lu random characters to %s", num_chars, file_path);
}