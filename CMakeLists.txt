cmake_minimum_required(VERSION 3.10)
project(netplayroom-manager)

add_definitions("-std=c++17")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(np-room-manager src/main.cpp)
target_link_libraries(np-room-manager ${CONAN_LIBS})

