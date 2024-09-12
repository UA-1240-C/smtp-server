add_library(SocketWrapper STATIC
        src/SocketWrapper.cpp
        include/TcpSocketManager.h
        include/TlsSocketManager.h
        src/TlsSocketManager.cpp
        src/TcpSocketManager.cpp
)

target_link_libraries(SocketWrapper PRIVATE Boost::log Boost::log_setup)

target_include_directories(SocketWrapper PUBLIC
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
)
