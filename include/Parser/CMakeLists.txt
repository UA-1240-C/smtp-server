add_library(Parser STATIC
        src/JSONParser.cpp
        src/ServerConfig.cpp
)

target_include_directories(Parser PUBLIC
        ${CMAKE_SOURCE_DIR}/include/Parser/include
)
