file(GLOB SOURCES "*.cpp")
file(GLOB TEST_SOURCES "test/*.cpp")

idf_component_register(SRCS "${SOURCES}"
                            "${TEST_SOURCES}"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES fatfs catch2 perfmon log
                    WHOLE_ARCHIVE)