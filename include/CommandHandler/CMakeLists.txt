add_library(CommandHandler src/CommandHandler.cpp
        src/StandartSmtpResponses.cpp
        include/MailMessageForwarder.h
        src/MailMessageSender.cpp)

target_link_libraries(CommandHandler PRIVATE
        MailDB
        Utils
        Boost::log
        Boost::log_setup
        ${OPENSSL_LIBRARIES})


target_link_libraries(CommandHandler PRIVATE MailDB)

target_include_directories(CommandHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/MailMessage/include
        ${CMAKE_SOURCE_DIR}/include/DataBase/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
)