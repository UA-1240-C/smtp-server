cmake_minimum_required(VERSION 3.13)
project(MailDBLibrary)

include(FetchContent)

set(PROJECT_NAME MailDBLibrary)
set(LIBRARY_NAME MailDB)
set(LIBRARY_HEADERS_DIR include/${LIBRARY_NAME})
set(LIBRARY_HEADERS
        ${LIBRARY_HEADERS_DIR}/IMailDB.h
        ${LIBRARY_HEADERS_DIR}/PgMailDB.h
        ${LIBRARY_HEADERS_DIR}/MailException.h
)

set(LIBRARY_SOURCE_DIR src)
set(LIBRARY_SOURCE ${LIBRARY_SOURCE_DIR}/PgMailDB.cpp)

FetchContent_Declare(
        libpqxx
        GIT_REPOSITORY https://github.com/jtv/libpqxx.git
        GIT_TAG master
)
FetchContent_MakeAvailable(libpqxx)

add_library(${LIBRARY_NAME} STATIC ${LIBRARY_HEADERS} ${LIBRARY_SOURCE})

target_link_libraries(${LIBRARY_NAME} PUBLIC pqxx)

target_include_directories(${LIBRARY_NAME} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
        $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>
)

target_include_directories(${LIBRARY_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)