cmake_minimum_required(VERSION 3.28)
project(SMTPLogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost 1.85 REQUIRED COMPONENTS log log_setup thread)
find_package(Threads REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

link_directories(${Boost_LIBRARYDIR})

add_executable(Logger main.cpp src/Logger.cpp)
target_include_directories(Logger PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS})

target_link_libraries(Logger Boost::log Boost::thread Boost::dynamic_linking Threads::Threads)