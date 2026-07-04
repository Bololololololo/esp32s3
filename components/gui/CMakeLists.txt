idf_component_register(SRCS "gui.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES lvgl utils esp_timer esp_driver_gpio
                     esp_driver_spi lvgl esp_lvgl_port esp_lcd_ili9341) 


