#include <MailMessageForwarder.h>
#include <ares.h>
#include <arpa/nameser.h>

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

void MailMessageForwarder::OnMXQueryComplete(void* arg, int status, int timeouts, struct ares_mx_reply* mx_reply)
{
    auto* mx_servers = static_cast<std::vector<std::string>*>(arg);
    if (status == ARES_SUCCESS && mx_reply != nullptr)
    {
        for (struct ares_mx_reply* mx = mx_reply; mx != nullptr; mx = mx->next)
        {
            mx_servers->push_back(mx->host);
        }
    }
    else
    {
        std::cerr << "Failed to resolve MX records: " << ares_strerror(status) << std::endl;
    }
    ares_free_data(mx_reply);
}

std::vector<std::string> MailMessageForwarder::ResolveMXRecords(const std::string& domain)
{
    std::vector<std::string> mx_servers;
    ares_channel channel;
    int status;

    // Initialize c-ares library
    status = ares_init(&channel);
    if (status != ARES_SUCCESS)
    {
        std::cerr << "Failed to initialize c-ares: " << ares_strerror(status) << std::endl;
        return mx_servers;
    }

    // Perform the MX query
    ares_query(
        channel, domain.c_str(), ns_c_in, ns_t_mx,
        [](void* arg, int status, int timeouts, unsigned char* abuf, int alen)
        {
            // Callback function needs to be defined to handle the raw data
            // For MX records, use ares_parse_mx_reply() to parse the data
            auto* mx_servers = static_cast<std::vector<std::string>*>(arg);
            if (status == ARES_SUCCESS)
            {
                struct ares_mx_reply* mx_reply = nullptr;
                int parse_status = ares_parse_mx_reply(abuf, alen, &mx_reply);
                if (parse_status == ARES_SUCCESS)
                {
                    for (struct ares_mx_reply* mx = mx_reply; mx != nullptr; mx = mx->next)
                    {
                        mx_servers->push_back(mx->host);
                    }
                    ares_free_data(mx_reply);
                }
                else
                {
                    std::cerr << "Failed to parse MX records: " << ares_strerror(parse_status) << std::endl;
                }
            }
            else
            {
                std::cerr << "Failed to resolve MX records: " << ares_strerror(status) << std::endl;
            }
        },
        &mx_servers);

    fd_set read_fds, write_fds, exc_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&exc_fds);

    struct timeval tv;
    tv.tv_sec = 5;  // Timeout of 5 seconds
    tv.tv_usec = 0;

    int max_fd;
    while ((max_fd = ares_fds(channel, &read_fds, &write_fds)) > 0)
    {
        int result = select(max_fd + 1, &read_fds, &write_fds, nullptr, &tv);
        if (result < 0)
        {
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            break;
        }
        else if (result > 0)
        {
            ares_process(channel, &read_fds, &write_fds);
        }
        else
        {
            std::cerr << "Timeout occurred" << std::endl;
            break;
        }
    }

    // Cleanup
    ares_destroy(channel);

    return mx_servers;
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

void OnHostQueryComplete(void* arg, int status, int timeouts, struct hostent* host)
{
    if (status == ARES_SUCCESS && host != nullptr)
    {
        char ip[INET6_ADDRSTRLEN];
        for (int i = 0; host->h_addr_list[i] != nullptr; ++i)
        {
            inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
            std::cout << "IP Address: " << ip << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to resolve host: " << ares_strerror(status) << std::endl;
    }
}

void ResolveIPAddresses(const std::vector<std::string>& mx_servers)
{
    ares_channel channel;
    int status;

    // Initialize c-ares library
    status = ares_init(&channel);
    if (status != ARES_SUCCESS)
    {
        std::cerr << "Failed to initialize c-ares: " << ares_strerror(status) << std::endl;
        return;
    }

    // Resolve IP addresses for each MX server
    for (const auto& mx_server : mx_servers)
    {
        ares_gethostbyname(channel, mx_server.c_str(), AF_INET, OnHostQueryComplete, nullptr);
    }

    // Wait for the queries to complete
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    struct timeval tv;
    tv.tv_sec = 5;  // Timeout of 5 seconds
    tv.tv_usec = 0;

    int max_fd;
    while ((max_fd = ares_fds(channel, &read_fds, &write_fds)) > 0)
    {
        int result = select(max_fd + 1, &read_fds, &write_fds, nullptr, &tv);
        if (result < 0)
        {
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            break;
        }
        else if (result > 0)
        {
            ares_process(channel, &read_fds, &write_fds);
        }
        else
        {
            std::cerr << "Timeout occurred" << std::endl;
            break;
        }
    }

    // Cleanup
    ares_destroy(channel);
}

bool MailMessageForwarder::ForwardEmailToClientServer(const ISXMM::MailMessage& message)
{
    for (const auto& recipient : message.to)
    {
        std::string recipient_domain = ExtractDomain(recipient.get_address());
        if (recipient_domain.empty())
        {
            Logger::LogError("Invalid recipient email address: " + recipient.get_address());
            continue;  // Skip to the next recipient
        }

        auto mx_servers = ResolveMXRecords(recipient_domain);
        if (mx_servers.empty())
        {
            Logger::LogError("Failed to resolve MX records for domain: " + recipient_domain);
            continue;  // Skip to the next recipient
        }

        // Iterate through MX servers and attempt to send the email
        for (const auto& mx_server : mx_servers)
        {
            try
            {
                std::cout << "mx_server: " << mx_server << std::endl;

                // Create a TCP socket and SSL context
                boost::asio::io_context io_context;
                boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv13_client);

                // Create an SSL socket and connect to the MX server
                auto ssl_socket =
                    std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(io_context, ssl_context);

                // Resolve endpoints with correct port (e.g., 465 for SSL)
                boost::asio::ip::tcp::resolver resolver(io_context);
                boost::asio::ip::tcp::resolver::query query("smtp.gmail.com", "587");
                auto endpoints = resolver.resolve(query);

                ResolveIPAddresses(mx_servers);


                std::cout << "connecting" << std::endl;
                boost::asio::connect(ssl_socket->lowest_layer(), endpoints);
				std::cout << "connected" << std::endl;
                ssl_socket->handshake(boost::asio::ssl::stream_base::client);

                // Wrap the socket in your socket wrapper
                ISXSocketWrapper::SocketWrapper socket_wrapper(ssl_socket);
                // Send SMTP commands
                if (SendSMTPCommands(socket_wrapper, message.from.get_address(), recipient.get_address(), message.body))
                {
                    Logger::LogProd("Email successfully forwarded to server: " + mx_server);
                    break;  // Stop trying once the email is successfully sent
                }
                Logger::LogError("Failed to send email to server: " + mx_server);
            }
            catch (const std::exception& e)
            {
                Logger::LogError("Exception while connecting to server: " + mx_server + ". Error: " + e.what());
            }
        }
    }

    return true;  // Return true if at least one email was successfully forwarded
}
