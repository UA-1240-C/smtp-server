add_library(CommandHandler src/CommandHandler.cpp)

target_compile_features(CommandHandler PRIVATE cxx_std_20)

target_link_libraries(CommandHandler PRIVATE MailDB)

target_include_directories(CommandHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
        ${CMAKE_SOURCE_DIR}/include/DataBase-main/libs/MailDB/include
        ${CMAKE_SOURCE_DIR}/include/ErrorHandler/include
)