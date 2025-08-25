cmake_minimum_required(VERSION 4.0)
project(Arduiono_Shield)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(CAN_Bus_Tester)
include_directories(main)

add_executable(Arduiono_Shield
        main/main.ino
        main/src/can_handler/can_handler.cpp
        main/src/can_handler/can_handler.h)
