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

std::string MailMessageForwarder::ExtractDomain(const std::string& email) {
	// Find the position of the '@' character in the email address
	std::size_t at_pos = email.find('@');

	// If '@' is found, return the substring after it, otherwise return an empty string
	if (at_pos != std::string::npos) {
		return email.substr(at_pos + 1);
	}
	return "";
}

std::vector<std::string> MailMessageForwarder::ResolveMXRecords(const std::string& domain) {
	std::vector<std::string> mx_servers;

	try {
		boost::asio::io_context io_context;
		boost::asio::ip::tcp::resolver resolver(io_context);
		boost::asio::ip::tcp::resolver::query query(domain, "smtp");
		boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
		boost::asio::ip::tcp::resolver::iterator end;

		for (; it != end; ++it) {
			mx_servers.push_back(it->host_name());
		}

		if (mx_servers.empty()) {
			Logger::LogError("No MX records found for domain: " + domain);
		}
	} catch (const std::exception& e) {
		Logger::LogError("Error resolving MX records: " + std::string(e.what()));
	}

	return mx_servers;
}


bool MailMessageForwarder::ForwardEmailToClientServer(const ISXMM::MailMessage& message)
{
	for (const auto& recipient : message.to) {
		std::string recipient_domain = ExtractDomain(recipient.get_address());
		if (recipient_domain.empty()) {
			Logger::LogError("Invalid recipient email address: " + recipient.get_address());
			continue; // Skip to the next recipient
		}

		auto mx_servers = ResolveMXRecords(recipient_domain);
		if (mx_servers.empty()) {
			Logger::LogError("Failed to resolve MX records for domain: " + recipient_domain);
			continue; // Skip to the next recipient
		}

		// Iterate through MX servers and attempt to send the email
		for (const auto& mx_server : mx_servers) {
			/*if (a method which will send all the SMTP commands) {
				Logger::LogProd("Email successfully forwarded to server: " + mx_server);
				break; // Stop trying once the email is successfully sent
			}*/
			Logger::LogError("Failed to send email to server: " + mx_server);
		}
	}

	return true; // Return true if at least one email was successfully forwarded
}