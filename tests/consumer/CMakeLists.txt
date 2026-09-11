cmake_minimum_required(VERSION 3.22)
project(wglib_consumer LANGUAGES CXX)

find_package(wglib CONFIG REQUIRED)

add_executable(wglib_consumer main.cpp)
target_link_libraries(wglib_consumer PRIVATE wglib::wglib)
