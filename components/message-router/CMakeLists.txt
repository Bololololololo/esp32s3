file(GLOB SOURCES "*.cpp")

set(extra_args "")

if(CONFIG_MSGR_TESTS)
    message(STATUS "Building message-router unit tests")
    set(extra_args "WHOLE_ARCHIVE")
    list(APPEND SOURCES "test/message_router_tests.cpp")
    list(APPEND SOURCES "test/message_router_integration_tests.cpp")
else()
    set(extra_args "")
endif()

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES log catch2
                       REQUIRES interfaces 
                       ${extra_args})