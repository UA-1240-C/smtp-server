add_library(Utils INTERFACE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(Utils INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)