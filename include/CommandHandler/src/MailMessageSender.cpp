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

void MailMessageForwarder::OnMXQueryComplete(void* arg, int status, ares_mx_reply* mx_reply)
{
    auto* mx_servers = static_cast<std::vector<std::string>*>(arg);
    if (status == ARES_SUCCESS && mx_reply != nullptr)
    {
        for (ares_mx_reply* mx = mx_reply; mx != nullptr; mx = mx->next)
        {
            mx_servers->emplace_back(mx->host);
        }
    }
    else
    {
        std::cerr << "Failed to resolve MX records: " << ares_strerror(status) << std::endl;
    }
    ares_free_data(mx_reply);   // clean the ares_mx_reply structure where we store resolved mx records
}

std::vector<std::string> MailMessageForwarder::ResolveMXRecords(const std::string& domain)
{
    std::vector<std::string> mx_servers;
    ares_channel channel; // manages a process of queries, including their creating and tracking

    // options for the channel set up
    ares_options options;
    int optmask = 0;  // a mask for options choosing

    // Initialize c-ares channel with options
    int status = ares_init_options(&channel, &options, optmask);

    // Initialize c-ares library
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
                ares_mx_reply* mx_reply = nullptr;
                int parse_status = ares_parse_mx_reply(abuf, alen, &mx_reply);
                if (parse_status == ARES_SUCCESS)
                {
                    for (ares_mx_reply* mx = mx_reply; mx != nullptr; mx = mx->next)
                    {
                        mx_servers->emplace_back(mx->host);
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

    timeval tv;
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

void OnHostQueryComplete(void* arg, int status, int timeouts, hostent* host)
{
    if (status == ARES_SUCCESS && host != nullptr)
    {
        std::cout << "Resolved IP addresses:" << std::endl;
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
    ares_channel channel; // manages a process of queries, including their creating and tracking

    // options for the channel set up
    ares_options options;
    int optmask = 0;  // a mask for options choosing

    // Initialize c-ares channel with options
    int status = ares_init_options(&channel, &options, optmask);
    if (status != ARES_SUCCESS)
    {
        std::cerr << "Failed to initialize c-ares: " << ares_strerror(status) << std::endl;
        return;
    }

    std::cout << "Resolving IP addresses for MX servers:" << std::endl;

    // Resolve IP addresses for each MX server
    for (const auto& mx_server : mx_servers)
    {
        ares_gethostbyname(channel, mx_server.c_str(), AF_INET, OnHostQueryComplete, nullptr);
    }

    // Wait for the queries to complete
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    timeval tv;
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
/*
bool MailMessageForwarder::ForwardEmailToClientServer(const ISXMM::MailMessage& message)
{
    // Initialize io_context, required for asynchronous operations
    boost::asio::io_context io_context;

    // Initialize the SSL context for TLS
    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::sslv23);
    ssl_context.set_default_verify_paths();  // Set default verification paths

    // Initialize the SocketWrapper with the io_context
    ISXSocketWrapper::SocketWrapper socket_wrapper(io_context);

    for (const auto& recipient : recipients)
    {
        auto mx_servers = ResolveMXRecords(recipient.get_domain());

        for (const auto& mx_server : mx_servers)
        {
            try
            {
                // Step 1: Connect to MX server
                auto connect_future = socketWrapper.Connect(mx_server, "smtp");
                io_context.run(); // Run the io_context to process async operations
                connect_future.get(); // Wait for connection to complete

                Logger::LogProd("Connected to MX server: " + mx_server);

                // Step 2: Perform TLS handshake
                auto tls_future = socketWrapper.PerformTlsHandshake(ssl_context, boost::asio::ssl::stream_base::client);
                io_context.restart(); // Reset io_context to reuse
                io_context.run();     // Run io_context for the TLS handshake
                tls_future.get();      // Wait for TLS handshake to complete

                Logger::LogProd("TLS handshake successful with: " + mx_server);

                // Step 3: Retrieve the SSL socket using std::get_if
                if (auto* ssl_socket = std::get_if<SslSocketPtr>(&socketWrapper.get_socket()))
                {
                    // Send SMTP commands using the SSL socket
                    if (SendSMTPCommands(*ssl_socket, message.from.get_address(), recipient.get_address(), message.body))
                    {
                        std::cout << "Email forwarded successfully to: " << recipient.get_address() << std::endl;
                        return true;
                    }
                    else
                    {
                        Logger::LogError("Failed to forward email to: " + recipient.get_address());
                    }
                }
                else
                {
                    Logger::LogError("No SSL socket available after TLS handshake.");
                }
            }
            catch (const std::exception& e)
            {
                Logger::LogError("Exception: " + std::string(e.what()) + " while forwarding to: " + mx_server);
            }
        }
    }

    return false;
}*/

#include <MailMessageForwarder.h>
#include <ares.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <stdexcept>

bool MailMessageForwarder::ForwardEmailToClientServer(const ISXMM::MailMessage& message) {
    // Extract domain from recipient email
    std::string recipient_domain = ExtractDomain(message.to[0].get_address());

    // Resolve MX records for the recipient's domain
    std::vector<std::string> mx_servers = ResolveMXRecords(recipient_domain);
    if (mx_servers.empty()) {
        Logger::LogError("No MX servers found for domain: " + recipient_domain);
        return false;
    }

    // Resolve IP addresses for the MX servers
    ResolveIPAddresses(mx_servers);

    // Use the first resolved MX server to connect
    std::string mx_server_ip = mx_servers.front();  // Assume we use the first MX server for simplicity
    boost::asio::io_context io_context;
    auto tcp_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
    ISXSocketWrapper::SocketWrapper socket_wrapper(tcp_socket);

    // Connect to the MX server
    auto connect_future = socket_wrapper.Connect(mx_server_ip, "smtp");
    try {
        connect_future.get();  // Wait for connection to complete
    } catch (const std::exception& e) {
        Logger::LogError("Connection error: " + std::string(e.what()));
        return false;
    }

    // Create SSL context
    boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_client);

    // Perform TLS handshake
    auto tls_future = socket_wrapper.PerformTlsHandshake(ssl_context, boost::asio::ssl::stream_base::client);
    try {
        tls_future.get();  // Wait for TLS handshake to complete
    } catch (const std::exception& e) {
        Logger::LogError("TLS handshake error: " + std::string(e.what()));
        return false;
    }

    // Send SMTP commands
    bool smtp_success = SendSMTPCommands(socket_wrapper,
        message.from.get_address(),
        message.to[0].get_address(),
        message.body);
    if (!smtp_success) {
        Logger::LogError("Failed to send SMTP commands.");
        return false;
    }

    return true;
}
