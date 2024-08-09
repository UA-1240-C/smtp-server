cmake_minimum_required(VERSION 3.13.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(MailDataBase)

# Source files
set(MAILDATABASE_SOURCE_DIR src)
set(MAILDATABASE_SOURCE
        ${MAILDATABASE_SOURCE_DIR}/main.cpp
)

# Libraries
set(MAILDATABASE_LIBRARIES_DIR libs)
set(MAILDATABASE_LIBRARIES
        MailDB
)

# Create executable
add_executable(${PROJECT_NAME} ${MAILDATABASE_SOURCE})

# Add subdirectories for libraries
foreach(LIBRARY ${MAILDATABASE_LIBRARIES})
    add_subdirectory("${MAILDATABASE_LIBRARIES_DIR}/${LIBRARY}")
endforeach()

# Link libraries
target_link_libraries(${PROJECT_NAME} ${MAILDATABASE_LIBRARIES})

# Fetch and add GoogleTest
include(FetchContent)

FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG v1.15.0)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

# Create test executable
add_executable(testRunner test/test.cpp)
target_link_libraries(testRunner ${MAILDATABASE_LIBRARIES} GTest::gtest_main GTest::gmock_main)

# Enable testing
enable_testing()