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
