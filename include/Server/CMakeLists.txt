add_library(Server src/Server.cpp)

target_link_libraries(Server PRIVATE
        MailDB
        CommandHandler
        MailMessage
        Encoding
        Boost::log
        Boost::log_setup
)

target_include_directories(Server PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
        ${CMAKE_SOURCE_DIR}/include/DataBase-main/libs/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/include/SignalHandler/include
        ${CMAKE_SOURCE_DIR}/include/Parser/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
)