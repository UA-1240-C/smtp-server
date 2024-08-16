add_library(SocketWrapper STATIC
        src/SocketWrapper.cpp
)

target_link_libraries(SocketWrapper PRIVATE Boost::log Boost::log_setup)

target_include_directories(SocketWrapper PUBLIC
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
)
