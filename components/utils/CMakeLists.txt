set(util_src "status_led.cpp"
             "system_info.cpp")    

idf_component_register(SRCS "${util_src}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_psram driver led_strip esp_hw_support esp_system)