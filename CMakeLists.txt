cmake_minimum_required(VERSION 4.3)
project(hailort-msmt)

set(CMAKE_CXX_STANDARD 26)

include_directories(include)

add_executable(hailort-msmt main.cpp)
find_package(Threads REQUIRED)
target_link_libraries(hailort-msmt hailort Threads::Threads)
