cmake_minimum_required(VERSION 3.16)

set(ENV{BLUEPAD32_ARDUINO} TRUE)
list(APPEND EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/third_party/bluepad32/src/components/bluepad32")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(inverse_kinematic_bluepad32)
