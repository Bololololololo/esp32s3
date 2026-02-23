idf_component_register(SRCS "main_app.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES status_led spi_flash lvgl lvgl_esp32_drivers lvgl_porting)