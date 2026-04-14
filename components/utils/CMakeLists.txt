file(GLOB SOURCES "*.cpp")

set(extra_args "")

if(CONFIG_UTILS_TESTS)
    message(STATUS "Building utils unit tests")
    set(extra_args "WHOLE_ARCHIVE")
    list(APPEND SOURCES "test/data_gen_unit_tests.cpp")
else()
    set(extra_args "")
endif()


idf_component_register(SRCS "${SOURCES}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_psram driver led_strip esp_hw_support esp_system catch2 storage interfaces
                       ${extra_args})