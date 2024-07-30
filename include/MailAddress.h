#pragma once

#include <string>

namespace ISXSC
{
    class MailAddress
    {
    public:
        MailAddress(std::string  address = "", std::string  name = "");
    
        explicit operator std::string() const;
    
        [[nodiscard]] std::string get_address() const;
        [[nodiscard]] std::string get_name() const;

        friend std::ostream& operator<<(std::ostream& os, const MailAddress& mailAddress);
    private:
        std::string address_;
        std::string name_;
    };
}