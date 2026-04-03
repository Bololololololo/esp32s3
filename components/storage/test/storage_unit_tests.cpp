#include "../include/storage.h"
#include "esp_log.h"
#include "xtensa_perfmon_apis.h"
#include <catch2/catch_test_macros.hpp>

static const char *TAG = "StorageUnitTests";

TEST_CASE("Storage: Basic Operations", "[storage]") {
    ESP_LOGI(TAG, "Running Storage: Basic Operations");
    Storage *storage = Storage::getInstance();
    esp_err_t ret;
    char buffer[64];

    // Test writting to a file
    ret = storage->write_file("/sdcard/test.txt", "Hello, World!");
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_OK);
    CHECK(strcmp(buffer, "Hello, World!") == 0);

    // Test overwriting the file
    ret = storage->write_file("/sdcard/test.txt", "Updated Content");
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_OK);
    CHECK(strcmp(buffer, "Updated Content") == 0);

    // Test appending to the file
    ret = storage->write_file("/sdcard/test.txt", " Appended Text", std::ios::app);
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_OK);
    CHECK(strcmp(buffer, "Updated Content Appended Text") == 0);

    ret = storage->delete_file("/sdcard/test.txt");
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_ERR_NOT_FOUND); // Expect file not found after deletion

    ret = storage->delete_file("/sdcard/test.txt");
    CHECK(ret == ESP_ERR_NOT_FOUND); // Expect file not found when trying to delete again
}

// 1. Define the function you want to profile
void my_test_function(void *params) {
    volatile int counter = 0;
    for (int i = 0; i < 1000; i++) {
        counter++;
    }
}

/* TBD fix result output as no output is seen from the callback result function */
TEST_CASE("Storage: example of performance testing") {
    ESP_LOGI(TAG, "Storage: example of performance testing");
    Storage *storage = Storage::getInstance();
    esp_err_t ret;

    // 2. Configure the performance monitor
    xtensa_perfmon_config_t config = {
        .repeat_count = 10,   // Number of times to run the function
        .max_deviation = 0.1, // Ignore results with high variance
        .call_params = storage,
        .call_function = Storage::writeWrapper,
        .callback = xtensa_perfmon_view_cb, // Built-in callback to print results
        .callback_params = stdout,
        .tracelevel = -1 // Only count if interrupt level > tracelevel (-1 = all)
    };

    ESP_LOGI(TAG, "Starting performance measurement...");

    // 3. Run the monitor for "Cycles" (select=0, mask=1)
    config.select_mask = (uint32_t[]){0x0001}; // Standard cycle counter
    config.counters_size = 1;
    xtensa_perfmon_exec(&config);

    // 4. Run the monitor for "Instructions" (select=1, mask=1)
    config.select_mask = (uint32_t[]){0x0101};
    xtensa_perfmon_exec(&config);
}