# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.12)

# Set the project name
project(Parser)

# Add the include directories
include_directories(include)

# Add the source files
set(SOURCES
        src/JSONParser.cpp
        src/ServerConfig.cpp
)
