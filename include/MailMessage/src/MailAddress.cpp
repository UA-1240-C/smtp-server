#include "MailAddress.h"

namespace ISXMM
{
MailAddress::MailAddress(const std::string& address, const std::string& name): m_address(address), m_name(name){}

std::string MailAddress::get_address() const
{
    return m_address;
}

std::string MailAddress::get_name() const
{
    return m_name;
}
} // namespace ISXMM