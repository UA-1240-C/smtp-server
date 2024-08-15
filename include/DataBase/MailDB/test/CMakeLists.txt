cmake_minimum_required(VERSION 3.13.0)

set(TEST_PROJECT_NAME
    MailDBTests
)
set(LIBRARY_TESTS_SOURCE
    test.cpp
)

project(${TEST_PROJECT_NAME})

enable_testing()

if (NOT (TARGET gtest_main AND TARGET gmock_main))
    message(FATAL_ERROR "gtest_main and gmock_main targets NOT found")
endif()

add_executable(${TEST_PROJECT_NAME} ${LIBRARY_TESTS_SOURCE})
target_link_libraries(${TEST_PROJECT_NAME} 
    gtest_main 
    gmock_main
    ${LIBRARY_NAME} # NOTE: This is defined from project above
)