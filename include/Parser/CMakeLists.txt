cmake_minimum_required(VERSION 3.13.0)

project(Parser)

set(HEADERS
        include/config.h
        include/parser.h
)

add_library(Parser INTERFACE)

target_include_directories(Parser INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(Parser INTERFACE cxx_std_20)