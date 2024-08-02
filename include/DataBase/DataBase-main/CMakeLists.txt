cmake_minimum_required(VERSION 3.13.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MAILDATABASE_PROJECT_NAME
    MailDataBase
)
set(MAILDATABASE_SOURCE_DIR
    src
)
set(MAILDATABASE_SOURCE
    ${MAILDATABASE_SOURCE_DIR}/main.cpp
)
set(MAILDATABASE_LIBRARIES_DIR
    libs
)

set(MAILDATABASE_LIBRARIES 
    MailDB
)

include(FetchContent)

project(${MAILDATABASE_PROJECT_NAME})

add_executable(${MAILDATABASE_PROJECT_NAME} ${MAILDATABASE_SOURCE})
set_property(TARGET ${MAILDATABASE_PROJECT_NAME} PROPERTY CXX_STANDARD 17)

foreach(LIBRARY ${MAILDATABASE_LIBRARIES})
    add_subdirectory("${MAILDATABASE_LIBRARIES_DIR}/${LIBRARY}")
endforeach(LIBRARY)

target_link_libraries(${MAILDATABASE_PROJECT_NAME} ${MAILDATABASE_LIBRARIES})



FetchContent_Declare(googletest
	GIT_REPOSITORY https://github.com/google/googletest
	GIT_TAG v1.15.0)
FetchContent_GetProperties(googletest)

if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
endif()

add_executable(testRunner test/test.cpp)
target_link_libraries(testRunner ${MAILDATABASE_LIBRARIES} gtest_main gmock_main)

