add_library(ErrorHandler INTERFACE)

target_link_libraries(ErrorHandler INTERFACE Boost::log Boost::log_setup)

target_include_directories(ErrorHandler INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
)