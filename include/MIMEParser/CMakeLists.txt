add_library(MIMEParser STATIC
        src/MIMEParser.cpp
)

target_link_libraries(MIMEParser PRIVATE 
        Utils
        Boost::log
        Boost::log_setup 
        vmime
)

target_include_directories(MIMEParser PUBLIC
        ${CMAKE_SOURCE_DIR}/include/MIMEParser/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
)