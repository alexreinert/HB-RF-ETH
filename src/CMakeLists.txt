# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

target_add_binary_data(${COMPONENT_TARGET} "../webui/dist/main.1e43358e.css.gz" BINARY)
target_add_binary_data(${COMPONENT_TARGET} "../webui/dist/main.1e43358e.js.gz" BINARY)
target_add_binary_data(${COMPONENT_TARGET} "../webui/dist/index.html.gz" BINARY)
target_add_binary_data(${COMPONENT_TARGET} "../webui/dist/favicon.26242483.ico.gz" BINARY)
