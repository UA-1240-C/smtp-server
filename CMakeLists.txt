cmake_minimum_required(VERSION 3.28)
project(SMTP_server)

set(CMAKE_CXX_STANDARD 20)

# Find Boost and OpenSSL
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# Add subdirectories
add_subdirectory(include/DataBase/DataBase-main/libs/MailDB)

# Add executable
add_executable(SMTP_server
        main.cpp
        src/Server.cpp
        src/MailAddress.cpp
        src/MailAttachment.cpp
        src/MailMessageBuilder.cpp
        src/Base64.cpp
        src/SignalHandler.cpp
)

# Link libraries
target_link_libraries(SMTP_server
        Boost::system
        Boost::thread
        ${OPENSSL_LIBRARIES}
        MailDB
        pqxx
        ${LIBSODIUM_LIBRARIES}  # Add libsodium
)

# Include directories
target_include_directories(SMTP_server PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/DataBase/DataBase-main/libs/MailDB
        ${LIBSODIUM_INCLUDE_DIRS}  # Add libsodium include directories
)
