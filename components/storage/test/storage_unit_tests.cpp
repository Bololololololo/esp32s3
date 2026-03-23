#include <catch2/catch_test_macros.hpp>
#include "../include/storage.h"

TEST_CASE("Storage: Basic Operations", "[storage]")
{
    Storage *storage = Storage::getInstance();
    esp_err_t ret;
    char buffer[64];

    // Test adding items
    ret = storage->write_file("/sdcard/test.txt", "Hello, World!");
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_OK);
    CHECK(strcmp(buffer, "Hello, World!") == 0);

    ret = storage->write_file("/sdcard/test.txt", "Updated Content");
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_OK);
    CHECK(strcmp(buffer, "Updated Content") == 0);

    ret = storage->delete_file("/sdcard/test.txt");
    CHECK(ret == ESP_OK);

    ret = storage->read_file("/sdcard/test.txt", buffer, sizeof(buffer));
    CHECK(ret == ESP_ERR_NOT_FOUND); // Expect file not found after deletion

    ret = storage->delete_file("/sdcard/test.txt");
    CHECK(ret == ESP_ERR_NOT_FOUND); // Expect file not found when trying to delete again
}