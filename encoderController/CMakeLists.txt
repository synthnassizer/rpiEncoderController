cmake_minimum_required(VERSION 3.1)
enable_language(C)

project(encoderController)

find_package(Threads REQUIRED)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

add_executable(encoderController
    encoderControllerMain.c
    jackMidiClient.c
    jackMidiClient.h
    rotaryEncoderClient.c
    rotaryEncoderClient.h
)

target_link_libraries(encoderController
    pigpiod_if2
    rt
    jack
    ${CMAKE_THREAD_LIBS_INIT}
)
