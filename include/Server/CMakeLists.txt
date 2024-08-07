add_library(Server src/Server.cpp)

target_include_directories(Server PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
        ${CMAKE_SOURCE_DIR}/include/DataBase-main/libs/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include

)