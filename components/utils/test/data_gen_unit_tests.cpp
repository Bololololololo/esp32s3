#include "../include/data_generator.h"
#include "esp_log.h"
#include "storage.h"
#include <catch2/catch_test_macros.hpp>
#include <fstream>

static const char *TAG = "DataGeneratorUnitTests";

TEST_CASE("Data generator: Basic Operations", "[data generator]") {
    ESP_LOGI(TAG, "Data generator: Basic Operations");
    Storage *storage = Storage::getInstance();
    int size_of_data = 100;
    esp_err_t ret;
    char buffer[size_of_data];

    const char *file_path = "/sdcard/test_random_data.txt";

    write_random_char_data(file_path, size_of_data);

    std::ifstream file(file_path);

    if (!file.is_open()) {
        FAIL("Failed to open file for reading");
    }

    file.read(buffer, sizeof(buffer));
    ESP_LOGI(TAG, "Read %lu bytes from file", file.gcount());
    ESP_LOGI(TAG, "Buffer content (hex): %s", std::string(buffer, file.gcount()).c_str());
    std::streamsize bytes_read = file.gcount();
    REQUIRE(bytes_read == size_of_data);

    file.close();
}