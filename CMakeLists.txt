cmake_minimum_required(VERSION 3.28)
project(Logger)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost REQUIRED COMPONENTS log log_setup thread unit_test_framework)
find_package(Threads REQUIRED)

# Create the library
add_library(Logger STATIC
    src/Logger.cpp
)

# Link libraries
target_link_libraries(Logger PUBLIC 
    Boost::log
    Boost::log_setup
    Boost::dynamic_linking
    Boost::thread 
    Threads::Threads
)

# Specify include directories for this library
target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

add_executable(TestLogger tests/TestLogger.cpp)
target_link_libraries(TestLogger PRIVATE
    Boost::unit_test_framework
    Logger
)
target_compile_definitions(TestLogger PRIVATE BOOST_TEST_MODULE=TestLogger)