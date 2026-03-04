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

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
