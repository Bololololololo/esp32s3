file(GLOB SOURCES "*.cpp")
file(GLOB TEST_SOURCES "test/*.cpp")

idf_component_register(SRCS "data_generator.cpp" "${SOURCES}"
                            "${TEST_SOURCES}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_psram driver led_strip esp_hw_support esp_system catch2 storage
                    WHOLE_ARCHIVE)