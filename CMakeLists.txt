cmake_minimum_required(VERSION 3.28)
project(SMTP_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
            GIT_TAG boost-1.86.0
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

# Disable building tests for libpqxx
if (NOT TARGET pqxx)
    FetchContent_Declare(
            libpqxx
            GIT_REPOSITORY https://github.com/jtv/libpqxx.git
            GIT_TAG 7.9.2
    )

    # Disable tests for libpqxx
    set(SKIP_BUILD_TEST ON CACHE BOOL "Skip build tests" FORCE)

    FetchContent_MakeAvailable(libpqxx)
endif()

# Add google tests
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.15.0)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
endif()

FetchContent_Declare(
    cares
    GIT_REPOSITORY https://github.com/c-ares/c-ares.git
    GIT_TAG v1.33
)
FetchContent_MakeAvailable(cares)


# Add subdirectories
add_subdirectory(include/CommandHandler)
add_subdirectory(include/DataBase/MailDB)
add_subdirectory(include/Server)
add_subdirectory(include/SignalHandler)
add_subdirectory(include/SocketWrapper)
add_subdirectory(include/MailMessage)
add_subdirectory(include/Utils)
add_subdirectory(include/Parser)
add_subdirectory(include/Logger)

# Add executable
add_executable(SMTP_server src/main.cpp)

# Link libraries
target_link_libraries(SMTP_server PRIVATE
    CommandHandler
    MailMessage
    MailDB
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
    c-ares
    ${OPENSSL_LIBRARIES}
)

# Include directories
target_include_directories(SMTP_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
    ${CMAKE_SOURCE_DIR}/include/DataBase/include
    ${CMAKE_SOURCE_DIR}/include/MailMessage/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
    ${CMAKE_SOURCE_DIR}/include/Server/include
    ${CMAKE_SOURCE_DIR}/include/SignalHandler/include
    ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
    ${CMAKE_SOURCE_DIR}/include/Utils/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBSODIUM_INCLUDE_DIRS}
)
