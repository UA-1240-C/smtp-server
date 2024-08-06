cmake_minimum_required(VERSION 3.28)
project(SMTPLogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost 1.85 REQUIRED COMPONENTS log thread)
if(Boost_FOUND)
    message(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARYDIR})
else()
    message(FATAL_ERROR "Could not find Boost")
endif()
find_package(Threads REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(Logger src/Logger.cpp)
target_include_directories(Logger PRIVATE ${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})

target_link_libraries(Logger Boost::log Boost::thread Threads::Threads)