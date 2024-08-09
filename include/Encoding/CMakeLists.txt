add_library(Encoding STATIC src/Base64.cpp)

target_include_directories(Encoding PUBLIC
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
)