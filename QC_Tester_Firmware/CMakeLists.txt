cmake_minimum_required(VERSION 4.0)
project(QC_Tester_Firmware)

set(CMAKE_CXX_STANDARD 14)

include_directories(main)
include_directories(main/src)
include_directories(main/src/channel_a)
include_directories(main/src/channel_b)
include_directories(main/src/comms)
include_directories(main/src/config)
include_directories(main/src/roles)
include_directories(main/src/slave_flasher)

add_executable(QC_Tester_Firmware
        main/src/channel_a/channel_a.cpp
        main/src/channel_a/channel_a.h
        main/src/channel_b/channel_b.cpp
        main/src/channel_b/channel_b.h
        main/src/comms/comms.cpp
        main/src/comms/comms.h
        main/src/config/config.h
        main/src/roles/roles.h
        main/src/slave_flasher/slave_flasher.cpp
        main/src/slave_flasher/slave_flasher.h
        main/main.ino
        main/src/cic_control/cic_adc.cpp
        main/src/cic_control/cic_adc.h)
