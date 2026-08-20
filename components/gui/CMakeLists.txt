file(GLOB_RECURSE LVGL_UI_SRCS
    "lvgl_ui_editor/*.c"
)
# Exclude the preview-build stub which is not for target builds
list(FILTER LVGL_UI_SRCS EXCLUDE REGEX ".*/preview-build/.*")

idf_component_register(SRCS "gui.cpp" ${LVGL_UI_SRCS}
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES lvgl utils esp_timer esp_driver_gpio
                     esp_driver_spi esp_lvgl_port esp_lcd_ili9341
                     esp_lcd_touch_ft6x36 driver)

target_compile_definitions(__idf_gui PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)

    