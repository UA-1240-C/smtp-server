#!/bin/bash

if [ ! -f CMakeLists.txt ]; then
  echo "Error: CMakeLists.txt not found. Please run this script from the root directory of the project."
  exit 1
fi

BUILD_DIR="build"

if [ -d "$BUILD_DIR" ]; then
  echo "Removing existing build directory..."
  rm -rf "$BUILD_DIR"
fi

mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring the project with CMake..."
cmake ..

echo "Building the project..."
make

if [ $? -eq 0 ]; then
    echo "Build completed successfully."
else
    echo "Build failed."
    exit 1
fi