cmake_minimum_required(VERSION 3.0.0)

project(peektur)

find_package(Boost 1.58 COMPONENTS filesystem REQUIRED)
find_package(ImageMagick COMPONENTS Magick++)
find_package(Threads REQUIRED)

add_definitions(-std=c++11)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${ImageMagick_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/deps/json/src)

add_executable(peektur main.cpp)
target_link_libraries(peektur ${Boost_LIBRARIES} ${ImageMagick_LIBRARIES} Threads::Threads)

