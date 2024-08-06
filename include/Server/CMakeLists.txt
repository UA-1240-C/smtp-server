cmake_minimum_required(VERSION 3.13.0)

project(Server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_FILES
        src/Server.cpp
)

add_library(Server ${SOURCE_FILES})

target_include_directories(Server PRIVATE
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
        ${CMAKE_SOURCE_DIR}/include/DataBase-main/libs/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include

)

target_compile_features(Server PRIVATE cxx_std_20)
