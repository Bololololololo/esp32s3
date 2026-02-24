/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "status_led.h"

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "esp_timer.h"
#include "lv_demos.h"

static const char *TAG = "main_app";

static void inc_lvgl_tick(void *arg)
{
    lv_tick_inc(10);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Main APP started");
    /* Configure the peripheral according to the LED type */
    configure_status_led();
    set_led_color(STATUS_LED_COLOR_RED);

    /* Initialize display*/
    lv_init();            // init lvgl
    lv_port_disp_init();  // init display
    lv_port_indev_init(); // init touch screen

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &inc_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 10 * 1000));

    lv_demo_widgets(); // LVGL Demo

    set_led_color(STATUS_LED_COLOR_GREEN);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
    }
}
