cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost REQUIRED COMPONENTS log log_setup thread)
find_package(Threads REQUIRED)

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

    # CMAKE_SOURCE_DIR is the root directory of the SMTP project, not just Logger
    ${CMAKE_SOURCE_DIR}/include/Utils/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
)
