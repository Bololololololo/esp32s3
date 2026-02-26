/*
 * This header file declares functions for controlling a status LED.
 * The LED can be configured to blink using either a GPIO or an addressable LED strip.
 * The implementation details are in the corresponding source file.
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

typedef enum
{
    STATUS_LED_COLOR_OFF = 0,
    STATUS_LED_COLOR_RED,
    STATUS_LED_COLOR_GREEN,
    STATUS_LED_COLOR_BLUE,
    STATUS_LED_COLOR_YELLOW,
} status_led_color_t;

void configure_status_led(void);
void set_led_color(status_led_color_t led_color);

#endif // STATUS_LED_H
