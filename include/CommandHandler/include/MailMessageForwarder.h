#ifndef MAILMESSAGEFORWARDER_H
#define MAILMESSAGEFORWARDER_H

#include <string>

#include <MailMessage.h>
#include <SocketWrapper.h>

#include "MailDB/PgMailDB.h"

class MailMessageForwarder
{
public:
	MailMessageForwarder(std::string server_domain);

	bool ForwardEmailToClientServer(const ISXMM::MailMessage& message);
private:
	std::string ExtractDomain(const std::string& email);
	std::vector<std::string> ResolveMXRecords(const std::string& domain);

	bool SendHELO(ISXSocketWrapper::SocketWrapper& socket_wrapper);
	bool SendMAILFROM(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& sender_email);
	bool SendRCPTTO(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& recipient_email);
	bool SendDATA(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& email_body);
	void SendQUIT(ISXSocketWrapper::SocketWrapper& socket_wrapper);

	bool SendSMTPCommands(ISXSocketWrapper::SocketWrapper& socket_wrapper, const std::string& sender_email, const std::string& recipient_email, const std::string& email_body);

    void OnMXQueryComplete(void* arg, int status, int timeouts, struct ares_mx_reply* mx_reply);

private : std::string m_server_domain;
    boost::asio::io_context m_io_context; 
};

#endif //MAILMESSAGEFORWARDER_H
