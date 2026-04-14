#include <fstream>
#include <iostream>

#include "driver/sdmmc_host.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "message-router.h"
#include "sdmmc_cmd.h"
#include "storage.h"

static const char *TAG = "sdmmc";

#define MOUNT_POINT "/sdcard"
#define SD_DATA_LINES 4

using namespace messageRouter;

esp_err_t Storage::mount() {
    esp_err_t ret{ESP_FAIL};

    // 1. Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false, .max_files = 5, .allocation_unit_size = 16 * 1024};

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;

    // 2. Initialize the SDMMC Host
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // 3. Define the Slot/Pin configuration for S3
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    /*
     * overrite driver configuration
     * we can patch the driver later
     */
    slot_config.clk = GPIO_NUM_38;
    slot_config.cmd = GPIO_NUM_40;
    slot_config.d0 = GPIO_NUM_39;
    slot_config.d1 = GPIO_NUM_41;
    slot_config.d2 = GPIO_NUM_48;
    slot_config.d3 = GPIO_NUM_47;
    slot_config.width = SD_DATA_LINES; // 4-bit mode for speed

    // 4. Mount the drive
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card. Error: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // 5. Get Card Capacity (Raw Hardware Info)
    // Capacity is in sectors.
    uint64_t total_bytes = ((uint64_t)card->csd.capacity) * card->csd.sector_size;
    ESP_LOGI(TAG, "SD Card Mounted successfully!\n");
    ESP_LOGI(TAG, "Type: %s\n", (card->is_sdio ? "SDIO" : (card->is_mmc ? "MMC" : "SDSC/SDHC")));
    ESP_LOGI(TAG, "Capacity: %llu MB\n", total_bytes / (1024 * 1024));

    // 6. Get Filesystem Info (Available Space)
    FATFS *fs;
    DWORD fre_clust;
    f_getfree("0:", &fre_clust, &fs);

    total_sect = (fs->n_fatent - 2) * fs->csize;
    free_sect = fre_clust * fs->csize;

    ESP_LOGI(TAG, "Free Space: %u MB / %u MB\n", (free_sect / 2048), (total_sect / 2048));
    return ESP_OK;
}

esp_err_t Storage::unmount() {
    esp_err_t ret{ESP_FAIL};
    ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);

    if (card == nullptr) {
        ESP_LOGE(TAG, "SD card was not mounted, skipping unmount.");
        return ESP_FAIL;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card. Error: %s\n", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SD Card unmounted successfully!\n");

    return ESP_OK;
}

esp_err_t Storage::write_file(const char *path, char *data, std::ios_base::openmode mode) {
    ESP_LOGI(TAG, "Opening file %s", path);

    std::ofstream outFile(path, std::ios::binary | mode);

    if (!outFile.is_open()) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    outFile << data;
    outFile.close();

    ESP_LOGI(TAG, "File written");
    return ESP_OK;
}

esp_err_t Storage::read_file(const char *path, char *buffer, size_t buffer_size) {
    ESP_LOGI(TAG, "Reading file %s", path);

    std::ifstream inFile(path);
    if (!inFile.is_open()) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_ERR_NOT_FOUND;
    }

    inFile.read(buffer, buffer_size - 1); // Leave space for null terminator
    buffer[inFile.gcount()] = '\0';       // Null-terminate the buffer
    inFile.close();

    ESP_LOGI(TAG, "Read from file: '%s'", buffer);
    return ESP_OK;
}

esp_err_t Storage::delete_file(const char *path) {
    ESP_LOGI(TAG, "Deleting file %s", path);
    if (unlink(path) != 0) {
        ESP_LOGE(TAG, "Failed to delete file");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "File deleted successfully");
    return ESP_OK;
}

esp_err_t Storage::messageHandler(const Message &msg) {
    ESP_LOGI(TAG, "Storage received message: %s from id : %d", msg.payload, msg.id);
    // Handle the message as needed
    return ESP_OK;
}