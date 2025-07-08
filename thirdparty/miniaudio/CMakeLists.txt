cmake_minimum_required(VERSION 3.8)
project(miniaudio)

add_library(miniaudio)
target_sources(miniaudio PRIVATE src/miniaudio.cpp)
target_include_directories(miniaudio PUBLIC include)