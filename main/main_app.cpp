/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "cmd_catch2.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gprof.h"
#include "gui_component.h"
#include "sdkconfig.h"
#include "service_manager.h"
#include "status_led.h"
#include "storage.h"
#include "wifi_service.h"
#include <iostream>
#include <memory>

using namespace services;
using namespace storage;
using namespace wifi;

#define LOG_LEVEL ESP_LOG_INFO

using namespace utils;

static const char *TAG = "main_app";

extern "C" {
void app_main(void);
}

void app_main(void) {
    ESP_LOGI(TAG, "Main APP started");

    /* Enable profiling */
    // esp_gprof_init();

    /* Set log level  */
    esp_log_level_set("*", LOG_LEVEL);

    /* Configure the peripheral according to the LED type */
    StatusLED status_led;
    status_led.setColor(utils::StatusLEDColor::RED);

    /* Initialize external storage */
    Storage *storage = Storage::getInstance();
    assert(storage != NULL);
    esp_err_t ret = storage->mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card. Error: %s\n", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD card mounted successfully");
    }

    status_led.setColor(utils::StatusLEDColor::GREEN);

    /* Configure console */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "catch2>";
    repl_config.task_stack_size = 100000;

    /* Register commands */
    esp_console_register_help_command();
    register_catch2("test");

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    /* Stop profiling and send results */
    // ESP_ERROR_CHECK(esp_gprof_save());
    // esp_gprof_deinit();

    /* Start the WiFi service*/
    ServiceManager *serviceManager = ServiceManager::getInstance();
    // serviceManager->startService(ServiceType::SERVICE_MSGR);
    // serviceManager->startService(ServiceType::SERVICE_WIFI);
    // serviceManager->startService(ServiceType::SERVICE_GUI);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
