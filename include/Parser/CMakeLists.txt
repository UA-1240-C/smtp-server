# Create the static library Parser
add_library(Parser STATIC
    src/JSONParser.cpp
    src/ServerConfig.cpp
)

# Link the necessary libraries
#target_link_libraries(Parser PRIVATE Boost::log Boost::log_setup)

# Specify the include directories for Parser
target_include_directories(Parser PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Define the test executable
add_executable(ParserTests tests/ParserTests.cpp)

# Link the test executable with the Parser library and GoogleTest
target_link_libraries(ParserTests PRIVATE Parser GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ParserTests)



# add_executable(ParserTests tests/ParserTests.cpp)
# target_link_libraries(ParserTests PRIVATE
#     Boost::unit_test_framework
#     Logger
# )
# target_compile_definitions(ParserTests PRIVATE BOOST_TEST_MODULE=ParserTests)