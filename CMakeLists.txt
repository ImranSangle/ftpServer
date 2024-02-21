
cmake_minimum_required(VERSION 3.5)

project(ftpServer)

add_executable(ftpServer src/main.cpp src/stringFunctions.cpp)
target_include_directories(ftpServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(TARGETS ftpServer DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
