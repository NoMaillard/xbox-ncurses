cmake_minimum_required(VERSION 3.8)
project(D)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/controller.c
        src/controller.h
        src/usb_handler.c
        src/usb_handler.h
        src/xb.c)

add_executable(D ${SOURCE_FILES})