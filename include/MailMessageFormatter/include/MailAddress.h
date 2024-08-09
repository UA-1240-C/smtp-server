#pragma once

#include <string>

namespace ISXSC
{
    class MailAddress
    {
    public:
        MailAddress(const std::string& address = "", const std::string& name = "");
    
        operator std::string() const;
    
        std::string get_address() const;
        std::string get_name() const;
    private:
        std::string m_address;
        std::string m_name;
    };
}