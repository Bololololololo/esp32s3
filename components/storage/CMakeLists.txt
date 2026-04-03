file(GLOB SOURCES "*.cpp")

set(extra_args "")

if(CONFIG_STORAGE_TESTS)
    message(STATUS "Building storage unit tests")
    set(extra_args "WHOLE_ARCHIVE")
    list(APPEND SOURCES "test/storage_unit_tests.cpp")
else()
    set(extra_args "")
endif()

idf_component_register(SRCS "${SOURCES}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES fatfs catch2 perfmon log
                       ${extra_args})