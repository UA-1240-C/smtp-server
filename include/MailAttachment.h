#pragma once

#include <string>
#include <filesystem>

namespace ISXSC
{
    class MailAttachment
    {
    public:
        explicit MailAttachment(std::filesystem::path  path = "");
    
        [[nodiscard]] std::filesystem::path get_path() const;
        [[nodiscard]] std::string get_name() const;

        static constexpr uint32_t S_MAX_SIZE = 5 * 1024 * 1024;
    private:
        std::filesystem::path path_;
    };
}