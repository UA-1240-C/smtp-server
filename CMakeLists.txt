# CMakeList.txt : CMake project for Logger, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("SMTPLogger")
# Add source to this project's executable.
add_executable (Logger "src/Logger.cpp" "src/Logger.h")

# Define the root project directory
set(ROOT_PROJECT_DIR ${CMAKE_SOURCE_DIR})
# Define the log file path
set(SERVERLOG "${ROOT_PROJECT_DIR}/serverlog.txt")
# Pass the log file path as macro
add_definitions(-DSERVERLOG=\"${SERVERLOG}\")
