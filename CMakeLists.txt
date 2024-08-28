# Logger/CMakeLists.txt

cmake_minimum_required(VERSION 3.28)
project(Logger)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

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
    ${CMAKE_SOURCE_DIR}/include/Utils/include
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

add_executable(TestLogger tests/TestLogger.cpp)
target_link_libraries(TestLogger PRIVATE
    Boost::unit_test_framework
    Logger
)
target_compile_definitions(TestLogger PRIVATE BOOST_TEST_MODULE=TestLogger)
