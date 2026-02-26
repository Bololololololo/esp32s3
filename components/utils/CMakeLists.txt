set(util_src "system_info.c"
             "status_led.c")    

idf_component_register(SRCS "${util_src}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_psram driver led_strip esp_hw_support)