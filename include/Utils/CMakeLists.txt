cmake_minimum_required(VERSION 3.13.0)

project(Utils)

set(HEADERS
        include/ThreadPool.h
        include/ThreadSafeQueue.h
)

add_library(Utils INTERFACE)

target_include_directories(Utils INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(Utils INTERFACE cxx_std_20)
