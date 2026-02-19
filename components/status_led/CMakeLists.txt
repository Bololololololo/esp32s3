idf_component_register(SRCS "status_led.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES driver led_strip)  # Add any other required components here
