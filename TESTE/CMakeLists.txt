cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH nao definido. Abra o ESP-IDF PowerShell e execute idf.py build.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(adc_test_olimex)
