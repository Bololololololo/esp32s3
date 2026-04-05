#include "../include/data_generator.h"
#include "esp_log.h"
#include "storage.h"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

static const char *TAG = "DataGeneratorUnitTests";

TEST_CASE("Data generator: Basic Operations", "[data generator]") {
    ESP_LOGI(TAG, "Data generator: Basic Operations");
    Storage *storage = Storage::getInstance();
    int size_of_data = 100;
    char buffer[size_of_data];
    esp_err_t ret;

    const char *file_path = "/sdcard/test.txt";

    if (std::filesystem::exists(file_path)) {
        ret = storage->delete_file(file_path);
        REQUIRE(ret == ESP_OK);
    }

    write_random_char_data(file_path, size_of_data);

    std::ifstream file(file_path);

    if (!file.is_open()) {
        FAIL("Failed to open file for reading");
    }

    file.read(buffer, sizeof(buffer));
    std::streamsize bytes_read = file.gcount();
    file.close();

    REQUIRE(bytes_read == size_of_data);
}