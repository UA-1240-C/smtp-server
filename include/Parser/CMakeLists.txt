add_library(Parser INTERFACE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(Parser INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)