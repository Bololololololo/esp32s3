# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Force LVGL to use the project-specific config file instead of the default
# lvgl/lv_conf.h shipped inside the component. This ensures the PSRAM-backed
# memory allocator settings in 3rd_party/lv_conf.h are actually active.
add_compile_definitions(LV_CONF_PATH="${CMAKE_SOURCE_DIR}/3rd_party/lv_conf.h")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(Trial_1)

include(gprof)
