cmake_minimum_required(VERSION 4.3)
project(hailort-msmt)

set(CMAKE_CXX_STANDARD 26)

include_directories(include)

add_executable(hailort-msmt main.cpp)
TARGET_LINK_LIBRARIES(hailort-msmt libhailort.so.5.3.0)
