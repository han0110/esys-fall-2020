cmake_minimum_required(VERSION 3.13)

project(main VERSION 1.0)

add_executable(main main.cxx)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET main PROPERTY CXX_STANDARD 17)

find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
target_link_libraries(main ${WIRINGPI_LIBRARIES})
