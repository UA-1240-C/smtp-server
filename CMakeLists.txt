cmake_minimum_required(VERSION 3.28)
project(SMTP_server)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(SMTP_server main.cpp src/Server.cpp)

target_include_directories(SMTP_server PRIVATE ${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

target_link_libraries(SMTP_server Boost::system Boost::thread ${OPENSSL_LIBRARIES})
