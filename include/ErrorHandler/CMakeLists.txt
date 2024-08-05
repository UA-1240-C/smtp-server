set(HEADER
        include/ErrorHandler.h
)

add_library(ErrorHandler INTERFACE)

target_include_directories(ErrorHandler INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(ErrorHandler INTERFACE cxx_std_20)
