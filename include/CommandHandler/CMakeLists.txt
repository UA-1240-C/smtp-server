find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

add_library(CommandHandler src/CommandHandler.cpp
        src/StandartSmtpResponses.cpp
)

target_link_libraries(CommandHandler PRIVATE
        MailDB
        Utils
        Boost::log
        Boost::log_setup
        ${OPENSSL_LIBRARIES}
        ${JSONCPP_LIBRARIES}
)


target_link_libraries(CommandHandler PRIVATE 
        MailDB
        MIMEParser
)

target_include_directories(CommandHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/MailMessage/include
        ${CMAKE_SOURCE_DIR}/include/DataBase/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
        ${CMAKE_SOURCE_DIR}/include/Server/include
        ${CMAKE_SOURCE_DIR}/include/Utils/include
        ${CMAKE_SOURCE_DIR}/include/MIMEParser/include
        ${JSONCPP_INCLUDE_DIRS}
)
