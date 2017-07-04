cmake_minimum_required(VERSION 3.0.0)

project(peektur)

find_package(Boost 1.60 COMPONENTS filesystem REQUIRED)

add_definitions(-std=c++11)
add_definitions(-Dcimg_display=0)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/deps/CImg)
include_directories(${CMAKE_SOURCE_DIR}/deps/json/src)

add_executable(peektur main.cpp)
target_link_libraries(peektur ${Boost_LIBRARIES})
