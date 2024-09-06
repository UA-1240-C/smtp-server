#include <MailMessageForwarder.h>

MailMessageForwarder::MailMessageForwarder(std::string server_domain)
	: m_server_domain(std::move(server_domain)) {}

bool MailMessageForwarder::SendHELO(ISXSocketWrapper::SocketWrapper& socket_wrapper)
{
	std::string command = "HELO " + m_server_domain + "\r\n";
	socket_wrapper.SendResponseAsync(command).get();
	std::string response = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();
	return response.substr(0, 3) == "250";
}

bool MailMessageForwarder::SendMAILFROM(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& sender_email)
{
	std::string command = "MAIL FROM:<" + sender_email + ">\r\n";
	socket_wrapper.SendResponseAsync(command).get();
	std::string response = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();
	return response.substr(0, 3) == "250";
}

bool MailMessageForwarder::SendRCPTTO(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& recipient_email)
{
	std::string command = "RCPT TO:<" + recipient_email + ">\r\n";
	socket_wrapper.SendResponseAsync(command).get();
	std::string response = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();
	return response.substr(0, 3) == "250";
}

bool MailMessageForwarder::SendDATA(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& email_body)
{
	socket_wrapper.SendResponseAsync("DATA\r\n").get();
	std::string response = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();
	if (response.substr(0, 3) != "354") {
		return false;
	}

	// Send the email content and end with a single dot on a line
	socket_wrapper.SendResponseAsync(email_body + "\r\n.\r\n").get();
	response = socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();
	return response.substr(0, 3) == "250";
}

void MailMessageForwarder::SendQUIT(ISXSocketWrapper::SocketWrapper& socket_wrapper)
{
	socket_wrapper.SendResponseAsync("QUIT\r\n").get();
	socket_wrapper.ReadFromSocketAsync(MAX_LENGTH).get();  // Read the server's response
}

bool MailMessageForwarder::SendSMTPCommands(ISXSocketWrapper::SocketWrapper& socket_wrapper,
                                            const std::string& sender_email, const std::string& recipient_email,
                                            const std::string& email_body)
{
    if (!SendHELO(socket_wrapper))
    {
        Logger::LogError("HELO command failed.");
        return false;
    }

    if (!SendMAILFROM(socket_wrapper, sender_email))
    {
        Logger::LogError("MAIL FROM command failed.");
        return false;
    }

    if (!SendRCPTTO(socket_wrapper, recipient_email))
    {
        Logger::LogError("RCPT TO command failed.");
        return false;
    }

    if (!SendDATA(socket_wrapper, email_body))
    {
        Logger::LogError("DATA command failed.");
        return false;
    }

    SendQUIT(socket_wrapper);
    return true;
}

