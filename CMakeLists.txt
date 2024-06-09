cmake_minimum_required(VERSION 3.24)
project(png-to-pnm C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(png-to-pnm
        converter.c
        return_codes.h)
