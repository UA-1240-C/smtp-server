add_library(MailMessage
    src/MailAddress.cpp
    src/MailAttachment.cpp
    src/MailMessageBuilder.cpp
    src/MailMessageFormatter.cpp)

target_include_directories(MailMessage PUBLIC include)