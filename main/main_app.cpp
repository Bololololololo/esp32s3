/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <iostream>
#include <memory>

#include "esp_system.h"
#include "sdkconfig.h"

#include "esp_lvgl_port.h"
#include "esp_timer.h"
// #include "lv_port_indev.h"
#include "lvgl.h"

// #include "gui.h"
#include "status_led.h"
#include "storage.h"

#include "cmd_catch2.h"
#include "esp_console.h"

#include "gprof.h"

////////
#include <inttypes.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_lcd_ili9341.h"
///////////

#define LOG_LEVEL ESP_LOG_INFO

using namespace utils;

static const char *TAG = "main_app";

// Task created for GUI to run in separate core
void vTaskGui(void *pvParameters) {
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void inc_lvgl_tick(void *arg) {
    lv_tick_inc(1);
}

extern "C" {
void app_main(void);
}

#define TEST_LCD_HOST SPI2_HOST
#define TEST_LCD_H_RES (320)
#define TEST_LCD_V_RES (240)
#define TEST_LCD_BIT_PER_PIXEL (16)

#define TEST_PIN_NUM_LCD_CS (GPIO_NUM_10)
#define TEST_PIN_NUM_LCD_PCLK (GPIO_NUM_12)
#define TEST_PIN_NUM_LCD_DATA0 (GPIO_NUM_11)
#define TEST_PIN_NUM_LCD_DC (GPIO_NUM_46)
#if CONFIG_IDF_TARGET_ESP32S3
#define TEST_PIN_NUM_LCD_RST (GPIO_NUM_18)
#else
#define TEST_PIN_NUM_LCD_RST (GPIO_NUM_1)
#endif
#if CONFIG_IDF_TARGET_ESP32S3
#define TEST_PIN_NUM_LCD_BL (GPIO_NUM_45)
#else
#define TEST_PIN_NUM_LCD_BL (GPIO_NUM_0)
#endif

#define TEST_DELAY_TIME_MS (3000)

static SemaphoreHandle_t refresh_finish = NULL;

IRAM_ATTR static bool test_notify_refresh_ready(esp_lcd_panel_io_handle_t panel_io,
                                                esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    BaseType_t need_yield = pdFALSE;

    xSemaphoreGiveFromISR(refresh_finish, &need_yield);
    return (need_yield == pdTRUE);
}

static void test_draw_bitmap(esp_lcd_panel_handle_t panel_handle) {
    refresh_finish = xSemaphoreCreateBinary();

    uint16_t row_line = TEST_LCD_V_RES / TEST_LCD_BIT_PER_PIXEL;
    uint8_t byte_per_pixel = TEST_LCD_BIT_PER_PIXEL / 8;
    uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * TEST_LCD_H_RES * byte_per_pixel, MALLOC_CAP_DMA);

    // Red in RGB565 = 0xF800; panel is BGR-order so 0xF800 maps red to the R channel
    uint16_t red_pixel = SPI_SWAP_DATA_TX(0xF800, TEST_LCD_BIT_PER_PIXEL);
    for (int i = 0; i < row_line * TEST_LCD_H_RES; i++) {
        color[i * byte_per_pixel + 0] = (red_pixel >> 0) & 0xFF;
        color[i * byte_per_pixel + 1] = (red_pixel >> 8) & 0xFF;
    }

    for (int j = 0; j < TEST_LCD_BIT_PER_PIXEL; j++) {
        esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, TEST_LCD_H_RES, (j + 1) * row_line, color);
        xSemaphoreTake(refresh_finish, portMAX_DELAY);
    }
    free(color);
    vSemaphoreDelete(refresh_finish);
}

void app_lcd_test(void) {
    ESP_LOGI(TAG, "Turn on the backlight");
    gpio_config_t io_conf = {
        .pin_bit_mask = BIT64(TEST_PIN_NUM_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(TEST_PIN_NUM_LCD_BL, 1);

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .mosi_io_num = TEST_PIN_NUM_LCD_DATA0,
        .miso_io_num = -1,
        .sclk_io_num = TEST_PIN_NUM_LCD_PCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TEST_LCD_H_RES * 80 * TEST_LCD_BIT_PER_PIXEL / 8,
    };
    spi_bus_initialize(TEST_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config =
        ILI9341_PANEL_IO_SPI_CONFIG(TEST_PIN_NUM_LCD_CS, TEST_PIN_NUM_LCD_DC, test_notify_refresh_ready, NULL);
    // Attach the LCD to the SPI bus
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)TEST_LCD_HOST, &io_config, &io_handle);

    ESP_LOGI(TAG, "Install ili9341 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,
        .reset_gpio_num = TEST_PIN_NUM_LCD_RST,
    };
    esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_swap_xy(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, true);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_lcd_panel_disp_off(panel_handle, false);
#else
    esp_lcd_panel_disp_on_off(panel_handle, true);
#endif

    test_draw_bitmap(panel_handle);
    vTaskDelay(pdMS_TO_TICKS(TEST_DELAY_TIME_MS));
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

    /* Initialize display*/
    lv_init(); // init lvgl
    // const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    // esp_err_t err = lvgl_port_init(&lvgl_cfg);
    // lv_port_indev_init(); // init touch screen

    // app_lcd_init();
    // // app_touch_init();
    // app_lvgl_init(-1);
    // app_main_display();

    app_lcd_test();

    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &inc_lvgl_tick, .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1000));

    /* Initialize external storage */
    Storage *storage = Storage::getInstance();
    assert(storage != NULL);
    esp_err_t ret = storage->mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card. Error: %s\n", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD card mounted successfully");
    }

    /* Initialize the GUI */
    // Gui *gui = Gui::getInstance();
    // assert(gui != NULL);
    // gui->init();

    status_led.setColor(utils::StatusLEDColor::GREEN);

    // Run GUI task on a separate core to ensure smooth performance
    xTaskCreatePinnedToCore(vTaskGui,                 // Function
                            "Core1_Task",             // Name
                            8192,                     // Stack size
                            NULL,                     // Parameter
                            configMAX_PRIORITIES - 1, // Priority
                            NULL,                     // Handle
                            1                         // <--- Pinned to Core 1
    );

    // Configure console
    //     esp_console_repl_t *repl = NULL;
    //     esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    //     repl_config.prompt = "catch2>";
    //     repl_config.task_stack_size = 100000;

    //     /* Register commands */
    //     esp_console_register_help_command();
    //     register_catch2("test");

    // #if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    //     esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    //     ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

    // #elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    //     esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    //     ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

    // #elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    //     esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    //     ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

    // #else
    // #error Unsupported console type
    // #endif

    //     ESP_ERROR_CHECK(esp_console_start_repl(repl));

    /* Stop profiling and send results */
    // ESP_ERROR_CHECK(esp_gprof_save());
    // esp_gprof_deinit();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
