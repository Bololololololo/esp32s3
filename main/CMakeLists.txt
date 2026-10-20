idf_component_register(SRCS "main_app.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES lvgl esp_lvgl_port gui
                        utils storage console gprof interfaces sdmmc wifi service_manager)

target_compile_options(${COMPONENT_LIB} PUBLIC
                       "$<$<COMPILE_LANGUAGE:C>:-Wno-designated-init>"
                       "$<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>")