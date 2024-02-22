
cmake_minimum_required(VERSION 3.5)

project(ftpServer)

set(CMAKE_BUILD_TYPE Debug)

add_executable(ftpServer src/winmain.cpp src/stringFunctions.cpp)
target_include_directories(ftpServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ftpServer PRIVATE ws2_32)

install(TARGETS ftpServer DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)
