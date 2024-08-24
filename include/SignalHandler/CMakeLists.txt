add_library(SignalHandler STATIC
        src/SignalHandler.cpp
)

target_link_libraries(SignalHandler PRIVATE Boost::log Boost::log_setup)

target_include_directories(SignalHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/SignalHandler/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
)
