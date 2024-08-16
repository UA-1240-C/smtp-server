add_library(Encoding STATIC src/Base64.cpp)

target_link_libraries(Encoding PRIVATE Boost::log Boost::log_setup)

target_include_directories(Encoding PUBLIC
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
)