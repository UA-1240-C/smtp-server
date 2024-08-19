add_library(CommandHandler src/CommandHandler.cpp)

target_link_libraries(CommandHandler PRIVATE MailDB Boost::log Boost::log_setup)

target_include_directories(CommandHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
        ${CMAKE_SOURCE_DIR}/include/DataBase/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/ErrorHandler/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Server/include

)