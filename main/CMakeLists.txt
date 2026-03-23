idf_component_register(SRCS "main_app.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES lvgl lvgl_esp32_drivers lvgl_porting gui utils storage console)