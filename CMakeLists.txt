
cmake_minimum_required(VERSION 3.5)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

project(ftpServer)

include_directories(include)

add_executable(ftpServer src/main.cpp src/stringFunctions.cpp)

add_compile_options(-g)

install(TARGETS ftpServer DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
