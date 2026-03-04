/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <memory>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "status_led.h"

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "esp_timer.h"
#include "gui.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

using namespace utils;

static const char *TAG = "main_app";

// Task created for GUI to run in separate core
void vTaskGui(void *pvParameters)
{
    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void inc_lvgl_tick(void *arg)
{
    lv_tick_inc(10);
}

extern "C"
{
    void app_main(void);
}

void init_sd_card_and_get_info()
{
    esp_err_t ret;

    // 1. Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    sdmmc_card_t *card;
    const char mount_point[] = "/sdcard";

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
    slot_config.width = 4; // 4-bit mode for speed

    // 4. Mount the drive
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        ESP_LOGI(TAG, "Failed to mount SD card. Error: %s\n", esp_err_to_name(ret));
        return;
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

    uint32_t total_sect = (fs->n_fatent - 2) * fs->csize;
    uint32_t free_sect = fre_clust * fs->csize;

    ESP_LOGI(TAG, "Free Space: %u MB / %u MB\n", (free_sect / 2048), (total_sect / 2048));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Main APP started");
    /* Configure the peripheral according to the LED type */
    StatusLED status_led;
    status_led.setColor(utils::StatusLEDColor::RED);

    /* Initialize display*/
    lv_init();            // init lvgl
    lv_port_disp_init();  // init display
    lv_port_indev_init(); // init touch screen

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &inc_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1000));

    /* Initialize the GUI */
    Gui *gui = Gui::getInstance();
    assert(gui != NULL);
    gui->init();

    status_led.setColor(utils::StatusLEDColor::GREEN);

    // Run GUI task on a separate core to ensure smooth performance
    xTaskCreatePinnedToCore(
        vTaskGui,                 // Function
        "Core1_Task",             // Name
        8192,                     // Stack size
        NULL,                     // Parameter
        configMAX_PRIORITIES - 1, // Priority
        NULL,                     // Handle
        1                         // <--- Pinned to Core 1
    );

    /* init SD card and get info */
    init_sd_card_and_get_info();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
