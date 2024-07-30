#include "MailAttachment.h"

#include <string>
#include <utility>

namespace ISXSC
{
    MailAttachment::MailAttachment(std::filesystem::path path)
        : path_(std::move(path)){}

    std::filesystem::path MailAttachment::get_path() const
    {
        return path_;
    }

    std::string MailAttachment::get_name() const
    {
        return path_.filename().string();
    }
}