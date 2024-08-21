#pragma once

#include <string>
#include <filesystem>

namespace ISXMM
{
class MailAttachment
{
public:
    MailAttachment(const std::filesystem::path& path = "");

    std::filesystem::path get_path() const;
    std::string get_name() const;
    static inline constexpr uint32_t S_MAX_SIZE = 5 * 1024 * 1024;
private:
    std::filesystem::path m_path;
};
} // namespace ISXMM