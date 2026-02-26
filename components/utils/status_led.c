#include <stdio.h>
#include "status_led.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "rgb_led";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip = NULL;

void configure_status_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void set_led_color(status_led_color_t led_color)
{
    if (led_strip == NULL)
    {
        ESP_LOGE(TAG, "LED strip not initialized");
        return;
    }

    switch (led_color)
    {
    case STATUS_LED_COLOR_OFF:
        led_strip_clear(led_strip);
        break;
    case STATUS_LED_COLOR_RED:
        led_strip_set_pixel(led_strip, 0, 255, 0, 0);
        break;
    case STATUS_LED_COLOR_GREEN:
        led_strip_set_pixel(led_strip, 0, 0, 255, 0);
        break;
    case STATUS_LED_COLOR_BLUE:
        led_strip_set_pixel(led_strip, 0, 0, 0, 255);
        break;
    case STATUS_LED_COLOR_YELLOW:
        led_strip_set_pixel(led_strip, 0, 255, 255, 0);
        break;
    }
    led_strip_refresh(led_strip);
}

#else
#error "unsupported LED type"
#endif
