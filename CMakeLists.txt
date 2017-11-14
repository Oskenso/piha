cmake_minimum_required(VERSION 3.6)

project(display)

add_definitions(-std=c11 -O2 -march=native)

include_directories(include)

file(GLOB SOURCES "src/*.c")

add_executable(display ${SOURCES})

target_link_libraries(display m wiringPi)
