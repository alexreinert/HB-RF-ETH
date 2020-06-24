# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

FILE(GLOB_RECURSE embed_webui_files ${CMAKE_SOURCE_DIR}/src/webui/*.gzip)

foreach(file ${embed_webui_files})
  target_add_binary_data(${COMPONENT_TARGET} "${file}" TEXT)
endforeach(file)
