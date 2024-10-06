# Logger/CMakeLists.txt

cmake_minimum_required(VERSION 3.28)
# Correctly set the LOGFILE_PATH variable
set(LOGFILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/serverlog.txt")

# Correctly pass LOGFILE_PATH as a preprocessor definition
add_compile_definitions(LOGFILE_PATH=\"${LOGFILE_PATH}\")

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

add_subdirectory(tests)