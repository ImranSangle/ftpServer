
cmake_minimum_required(VERSION 3.20)

project(ftpServer)

enable_testing()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
add_definitions(-DDEBUG)
endif()

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
  set(OSFILES 
      "os/windows/src/sockets.cpp"
      "os/windows/src/common.cpp"
  )
endif()

if(LINUX OR ANDROID)
  set(OSFILES "os/linux/src/sockets.cpp")
endif()

add_executable(ftpServer 
src/main.cpp
src/stringFunctions.cpp
src/rfc_959.cpp
src/rfc_3659.cpp
${OSFILES})

if(WIN32)
target_include_directories(ftpServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/os/windows/include)
target_link_libraries(ftpServer PRIVATE ws2_32)
endif()

if(LINUX OR ANDROID)
target_include_directories(ftpServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/os/linux/include)
endif()

install(TARGETS ftpServer DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(test)
