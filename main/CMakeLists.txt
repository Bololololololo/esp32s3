idf_component_register(SRCS "main_app.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES lvgl esp_lvgl_port gui
                        utils storage console gprof interfaces esp_timer sdmmc)

target_compile_options(${COMPONENT_LIB} PUBLIC "-pg" "-Wno-designated-init")   
