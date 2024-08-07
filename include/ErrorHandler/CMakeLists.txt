add_library(ErrorHandler INTERFACE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(ErrorHandler INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)