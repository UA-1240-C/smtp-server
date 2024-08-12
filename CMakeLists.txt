cmake_minimum_required(VERSION 3.28)
project(SMTP_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# Add subdirectories
add_subdirectory(include/CommandHandler)
add_subdirectory(include/DataBase-main)
add_subdirectory(include/Encoding)
add_subdirectory(include/Server)
add_subdirectory(include/SignalHandler)
add_subdirectory(include/SocketWrapper)
add_subdirectory(include/MailMessageFormatter)
add_subdirectory(include/Utils)
add_subdirectory(include/ErrorHandler)

# Add executable
add_executable(SMTP_server src/main.cpp)

# Link libraries
target_link_libraries(SMTP_server PRIVATE
        ErrorHandler
        CommandHandler
        MailMessage
        MailDB
        Encoding
        Server
        SignalHandler
        SocketWrapper
        Utils
        Boost::system
        Boost::thread
        ${OPENSSL_LIBRARIES}
        libpqxx::pqxx
        ${LIBSODIUM_LIBRARIES}
)

# Include directories
target_include_directories(SMTP_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBSODIUM_INCLUDE_DIRS}
)


# Linux-specific install paths
install(TARGETS SMTP_server DESTINATION bin)
install(FILES config.txt DESTINATION etc)
install(DIRECTORY include/ DESTINATION include)
install(FILES README.md DESTINATION share/doc/smtp-server)

# Linux-specific settings
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PACKAGE_NAME "smtp-server")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_INSTALL_PREFIX "/usr/local")
include(CPack)