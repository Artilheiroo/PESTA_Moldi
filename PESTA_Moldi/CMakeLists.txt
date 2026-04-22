cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH nao definido. Abra o ESP-IDF PowerShell e execute novamente.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(machine_monitor_blink_test)
