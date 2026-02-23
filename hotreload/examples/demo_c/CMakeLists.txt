cmake_minimum_required(VERSION 3.16)
project(demo_c)

add_executable(demo_c main.c)
target_link_libraries(demo_c PRIVATE hotreload)
target_include_directories(demo_c PRIVATE ../../include)
