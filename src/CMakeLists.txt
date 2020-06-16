# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

target_add_binary_data(${COMPONENT_TARGET} webui/index.htm TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/about.htm TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/webui.js TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/jquery.min.js TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/vue.min.js TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/bootstrap.min.css TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/bootstrap-vue.min.css TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/bootstrap-vue.min.js TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/vuelidate.min.js TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/validators.min.js TEXT)
target_add_binary_data(${COMPONENT_TARGET} webui/favicon.ico TEXT)
