add_library(ClientSession STATIC
    src/ClientSession.cpp
    src/SmtpRequest.cpp
    src/SmtpResponse.cpp
)

target_include_directories(ClientSession PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/include/Logger/include
)

target_link_libraries(ClientSession
    CommandHandler
    MailMessage
    SocketWrapper
    Boost::log
    Boost::log_setup
)
