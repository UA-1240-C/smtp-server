cmake_minimum_required(VERSION 3.28)
project(SMTP_server)

set(CMAKE_CXX_STANDARD 20)

# Find Boost and OpenSSL
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# Add subdirectories
add_subdirectory(include/CommandHandler)
add_subdirectory(include/DataBase/DataBase-main)
add_subdirectory(include/Encoding)
add_subdirectory(include/MailMessageFormatter)
add_subdirectory(include/Server)
add_subdirectory(include/SignalHandler)
add_subdirectory(include/SocketWrapper)
add_subdirectory(include/Utils)

# Add executable
add_executable(SMTP_server src/main.cpp)

# Link libraries
target_link_libraries(SMTP_server PRIVATE
        CommandHandler
        DataBase-main
        Encoding
        MailMessageFormatter
        Server
        SignalHandler
        SocketWrapper
        Utils
        Boost::system
        Boost::thread
        ${OPENSSL_LIBRARIES}
        pqxx
        ${LIBSODIUM_LIBRARIES}
)

# Include directories
target_include_directories(SMTP_server PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${LIBSODIUM_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/CommandHandler/include
)
