cmake_minimum_required(VERSION 3.16)

set(ENV{BLUEPAD32_ARDUINO} TRUE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(inverse_kinematic_bluepad32)
