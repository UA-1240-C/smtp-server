cmake_minimum_required(VERSION 3.28)
project(SMTP_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system thread log log_setup)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

include(FetchContent)

# Fetch and build Boost if not found
if (NOT Boost_FOUND)
    FetchContent_Declare(
            boost
            GIT_REPOSITORY https://github.com/boostorg/boost.git
<<<<<<< HEAD
<<<<<<< HEAD
            GIT_TAG boost-1.79.0
=======
            GIT_TAG boost-1.85.0
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
=======
            GIT_TAG boost-1.85.0
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
    )
    FetchContent_MakeAvailable(boost)
endif()

# Fetch and build OpenSSL if not found
if (NOT OpenSSL_FOUND)
    FetchContent_Declare(
            openssl
            GIT_REPOSITORY https://github.com/openssl/openssl.git
            GIT_TAG OpenSSL_1_1_1k
    )
    FetchContent_MakeAvailable(openssl)
endif()

<<<<<<< HEAD
# Fetch and build libpqxx if not found
if (NOT TARGET pqxx::pqxx)
    FetchContent_Declare(
        libpqxx
        GIT_REPOSITORY https://github.com/jtv/libpqxx.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(libpqxx)
endif()

=======
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
# Add subdirectories
add_subdirectory(include/CommandHandler)
add_subdirectory(include/DataBase/MailDB)
add_subdirectory(include/Encoding)
add_subdirectory(include/Server)
add_subdirectory(include/SignalHandler)
add_subdirectory(include/SocketWrapper)
add_subdirectory(include/MailMessageFormatter)
add_subdirectory(include/Utils)
add_subdirectory(include/ErrorHandler)
add_subdirectory(include/Parser)
<<<<<<< HEAD
add_subdirectory(include/Logger)
=======
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)

# Add executable
add_executable(SMTP_server src/main.cpp)

# Link libraries
target_link_libraries(SMTP_server PRIVATE
<<<<<<< HEAD
    ErrorHandler
    CommandHandler
    MailMessage
    MailDB
    Encoding
    Server
    SignalHandler
    SocketWrapper
    Utils
    Parser
    Logger
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup
    ${OPENSSL_LIBRARIES}
    pqxx
    ${LIBSODIUM_LIBRARIES}
=======
        ErrorHandler
        CommandHandler
        MailMessage
        MailDB
        Encoding
        Server
        SignalHandler
        SocketWrapper
        Utils
        Parser
        Boost::system
        Boost::thread
        ${OPENSSL_LIBRARIES}
        pqxx
        ${LIBSODIUM_LIBRARIES}
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
)

# Include directories
target_include_directories(SMTP_server PRIVATE
<<<<<<< HEAD
=======
    ${CMAKE_SOURCE_DIR}/include/Logger/include
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
    ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
    ${CMAKE_SOURCE_DIR}/include/DataBase/include
    ${CMAKE_SOURCE_DIR}/include/Encoding/include
    ${CMAKE_SOURCE_DIR}/include/ErrorHandler/include
    ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
    ${CMAKE_SOURCE_DIR}/include/Server/include
    ${CMAKE_SOURCE_DIR}/include/SignalHandler/include
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Utils/include
<<<<<<< HEAD
    ${CMAKE_SOURCE_DIR}/include/Logger
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBSODIUM_INCLUDE_DIRS}
)
=======
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBSODIUM_INCLUDE_DIRS}
)
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
