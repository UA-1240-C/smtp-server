cmake_minimum_required(VERSION 3.13.0)

project(CommandHandler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_FILES
        src/CommandHandler.cpp
)

add_library(CommandHandler ${SOURCE_FILES})

target_link_libraries(CommandHandler PRIVATE MailDB)

target_include_directories(CommandHandler PRIVATE
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
        ${CMAKE_SOURCE_DIR}/include/DataBase-main/libs/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/ErrorHandler/include
)

target_compile_features(CommandHandler PRIVATE cxx_std_17)
