add_library(CommandHandler src/CommandHandler.cpp)

target_link_libraries(CommandHandler PRIVATE MailDB Boost::log Boost::log_setup)
target_compile_features(CommandHandler PRIVATE cxx_std_20)

target_include_directories(CommandHandler PUBLIC
        ${CMAKE_SOURCE_DIR}/include/CommandHandler/include
        ${CMAKE_SOURCE_DIR}/include/Encoding/include
        ${CMAKE_SOURCE_DIR}/include/SocketWrapper/include
        ${CMAKE_SOURCE_DIR}/include/MailMessageFormatter/include
<<<<<<< HEAD
<<<<<<< HEAD
        ${CMAKE_SOURCE_DIR}/include/DataBase/MailDB/include
=======
        ${CMAKE_SOURCE_DIR}/include/DataBas//MailDB/include
>>>>>>> 0568652 (Initial commit of actual project stage for logger integration)
=======
        ${CMAKE_SOURCE_DIR}/include/DataBase/MailDB/include
>>>>>>> 589285f (Integrated logger. Rewrote some documentation and refactored the code)
        ${CMAKE_SOURCE_DIR}/include/ErrorHandler/include
        ${CMAKE_SOURCE_DIR}/include/Logger/include
)