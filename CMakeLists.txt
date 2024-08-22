cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(GTest CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS log log_setup thread)
find_package(Threads REQUIRED)

enable_testing()

# Create the library
add_library(Logger STATIC
    src/Logger.cpp
)

# Link libraries
target_link_libraries(Logger PUBLIC 
    Boost::log
    Boost::log_setup
    Boost::thread 
    Threads::Threads
)

# Specify include directories for this library
target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/Utils/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
)

# Create the test executable
add_executable(run_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestsLogger.cpp
)

# Link test executable with Google Test
target_link_libraries(run_tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main)

# Specify include directories for tests
target_include_directories(run_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include)

# Register the test with CTest
add_test(NAME all_tests COMMAND run_tests)