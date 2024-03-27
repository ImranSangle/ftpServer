
cmake_minimum_required(VERSION 3.5)

project(ftpServer)

enable_testing()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_definitions(-DDEBUG)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(ftpServer 
src/winmain.cpp
src/sockets.cpp
src/stringFunctions.cpp)

target_include_directories(ftpServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
target_link_libraries(ftpServer PRIVATE ws2_32)
endif()

install(TARGETS ftpServer DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(test)
