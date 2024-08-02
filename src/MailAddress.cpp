#include "MailAddress.h"

#include <utility>

namespace ISXSC
{
    MailAddress::MailAddress(std::string  address, std::string  name)
        : address_(std::move(address)), name_(std::move(name)) {}
    
    MailAddress::operator std::string() const
    {
        if (!name_.empty())
        {
            return name_ + " <" + address_ + ">";
        }
        return address_;
    }

    std::string MailAddress::get_address() const
    {
        return address_;
    }

    std::string MailAddress::get_name() const
    {
        return name_;
    }

    std::ostream& operator<<(std::ostream& os, const MailAddress& mailAddress)
    {
        if (!mailAddress.name_.empty()) {
            os << mailAddress.name_ + " <" << mailAddress.address_ + ">";
        } else {
            os << mailAddress.address_;
        }
        return os;
    }
}