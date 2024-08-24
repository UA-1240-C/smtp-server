# Logger/CMakeLists.txt

cmake_minimum_required(VERSION 3.28)

# Create the library
add_library(Logger STATIC
    src/Logger.cpp
)

# Link libraries
target_link_libraries(Logger PUBLIC Boost::log Boost::thread Threads::Threads)

# Specify include directories for this library
target_include_directories(Logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Parser/include
    ${CMAKE_SOURCE_DIR}/include/Utils/include
    ${Boost_INCLUDE_DIRS}
)


