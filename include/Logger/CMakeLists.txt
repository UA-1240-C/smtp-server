# Logger/CMakeLists.txt

cmake_minimum_required(VERSION 3.28)
project(Logger)

# Include the path to Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Create the library
add_library(Logger STATIC
    src/Logger.cpp
)

# Specify include directories for this library
target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/Parser/include
)

# Link libraries
target_link_libraries(Logger PUBLIC 
    Boost::log
    Boost::log_setup
    Boost::dynamic_linking
    Boost::thread 
    Threads::Threads
)

# Add the test executable
add_executable(TestLogger tests/TestLogger.cpp)
target_link_libraries(TestLogger PRIVATE
    Logger
    ${GTEST_LIBRARIES}
)

# Enable testing
enable_testing()

# Add test cases
add_test(NAME TestLogger COMMAND TestLogger)