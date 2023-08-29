cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-wakeup-keypress)

idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)
