add_library(SignalHandler STATIC
        src/SignalHandler.cpp
)

target_include_directories(SignalHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/SignalHandler/include
)
