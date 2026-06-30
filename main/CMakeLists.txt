idf_component_register(SRCS "main_app.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES lvgl esp_lvgl_port gui
                        utils storage console gprof interfaces esp_timer)

target_link_libraries(${COMPONENT_LIB} PUBLIC "-pg")
