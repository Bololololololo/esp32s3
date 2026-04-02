file(GLOB SOURCES "*.cpp")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "include"
                    REQUIRES  bt nvs_flash utils led_strip)
