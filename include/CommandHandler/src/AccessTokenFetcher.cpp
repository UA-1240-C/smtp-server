#include "AccessTokenFetcher.h"
#include <iostream>

AccessTokenFetcher::AccessTokenFetcher() = default;
AccessTokenFetcher::~AccessTokenFetcher() = default;

std::string AccessTokenFetcher::UrlEncode(const std::string& value) const {
    std::ostringstream encoded;
    for (const auto& c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
    }
    return encoded.str();
}

void AccessTokenFetcher::HandleRequest(tcp::socket& socket) {
    beast::flat_buffer buffer;
    http::request<http::string_body> req;

    // Read the HTTP request
    http::read(socket, buffer, req);

    // Convert request target (URL path and query) to std::string
    std::string target(req.target());

    // Print the full URL (for debugging purposes)
    std::cout << "Redirected to: " << target << std::endl;

    // Check if the request is for the "/callback" path
    if (target.find("/callback") == 0) {
        // Look for the "code" parameter in the URL
        size_t code_pos = target.find("code=");
        if (code_pos != std::string::npos) {
            // Extract the code after "code="
            m_authorization_code = target.substr(code_pos + 5);
            m_authorization_code = m_authorization_code.substr(0, m_authorization_code.find("&"));  // Extract only the code part

            // Print the authorization code
            std::cout << "Authorization Code: " << m_authorization_code << std::endl;
        } else {
            std::cout << "Authorization code not found in URL." << std::endl;
        }

        // Send a basic HTTP response to the browser
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.body() = "Authorization received. You can close this window.";
        res.prepare_payload();
        http::write(socket, res);
    } else {
        // Ignore other requests like /favicon.ico
        std::cout << "Ignored request: " << target << std::endl;
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::content_type, "text/plain");
        res.body() = "Not found.";
        res.prepare_payload();
        http::write(socket, res);
    }

    // Shutdown the socket connection
    socket.shutdown(tcp::socket::shutdown_send);
}

int AccessTokenFetcher::OpenOAuthBrowser() const {
    std::string base_url = "https://accounts.google.com/o/oauth2/auth";
    std::string scope = "https://mail.google.com/";
    std::string response_type = "code";
    std::string access_type = "offline";
    std::string redirect_uri = "http://localhost:8000/callback";
    std::string client_id = "570952853562-5n34bmhjdsd6q7bovgf1g8ks6q6d930o.apps.googleusercontent.com";

    // Construct the OAuth URL
    std::ostringstream full_url;
    full_url << base_url
             << "?scope=" << UrlEncode(scope)
             << "&response_type=" << UrlEncode(response_type)
             << "&access_type=" << UrlEncode(access_type)
             << "&redirect_uri=" << UrlEncode(redirect_uri)
             << "&client_id=" << UrlEncode(client_id)
             << "&prompt=" << "consent";

    std::string encoded_url = full_url.str();
    std::cout << "OAuth URL: " << encoded_url << std::endl;

    // Open the URL in the default browser depending on the OS
    #if defined(__linux__)
        std::string command = "xdg-open " + encoded_url;
    #elif defined(__APPLE__)
        std::string command = "open '" + encoded_url + "'";
    #elif defined(_WIN32)
        std::string command = "start " + encoded_url;
    #else
        std::cerr << "Unsupported OS" << std::endl;
        return 1;
    #endif

    // Execute the system command to open the browser
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to open browser" << std::endl;
        return 1;
    }
    return 0;
}

std::string AccessTokenFetcher::ParseAccessToken(const std::string& json_response) const {
    std::string access_token;
    // Create a JSON reader and parse the response
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::istringstream s(json_response);
    std::string errs;
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        if (root.isMember("access_token")) {
            access_token = root["access_token"].asString();
        }
    } else {
        std::cerr << "Failed to parse JSON response: " << errs << std::endl;
    }
    return access_token;
}

void AccessTokenFetcher::ExchangeCodeForToken(const std::string& authorization_code) {
    try {
        asio::io_context ioc;
        ssl::context ctx(ssl::context::tls_client);

        // Load system CA certificates
        ctx.set_default_verify_paths();

        tcp::resolver resolver{ioc};
        ssl::stream<tcp::socket> stream{ioc, ctx};

        auto const results = resolver.resolve("oauth2.googleapis.com", "https");
        asio::connect(stream.next_layer(), results.begin(), results.end());

        // Perform SSL handshake
        stream.handshake(ssl::stream_base::client);

        // URL-encode request parameters
        std::string body = "client_id=" + UrlEncode("570952853562-5n34bmhjdsd6q7bovgf1g8ks6q6d930o.apps.googleusercontent.com") +
                           "&client_secret=" + UrlEncode("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") +
                           "&code=" + UrlEncode(authorization_code) +
                           "&grant_type=" + UrlEncode("authorization_code") +
                           "&redirect_uri=" + UrlEncode("http://localhost:8000/callback");

        // Set up the POST request
        http::request<http::string_body> req{http::verb::post, "/token", 11};
        req.set(http::field::host, "oauth2.googleapis.com");
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.body() = body;
        req.prepare_payload();

        // Send the request
        http::write(stream, req);

        // Receive the response
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        // Print the response
        std::string response_body = boost::beast::buffers_to_string(res.body().data());

        // Print the response body
        std::cout << "Response Body: " << response_body << std::endl;

        // Parse the access token from the JSON response
        m_access_token = ParseAccessToken(response_body);
        if (!m_access_token.empty()) {
            std::cout << "Access Token: " << m_access_token << std::endl;
        } else {
            std::cout << "Access Token not found in response." << std::endl;
        }
        beast::error_code ec;
        // Close the socket
        stream.next_layer().shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
        stream.next_layer().close(ec);
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int AccessTokenFetcher::FetchAccessToken() {
    // Open the OAuth URL in the browser
    if (OpenOAuthBrowser() != 0) {
        return 1;
    }

    // Initialize the Asio io_context and acceptor
    asio::io_context ioc;
    tcp::acceptor acceptor{ioc, tcp::endpoint(tcp::v4(), 8000)};

    // Accept a connection
    tcp::socket socket{ioc};
    acceptor.accept(socket);

    // Handle the request from the socket
    HandleRequest(socket);

    // Exchange the authorization code for an access token
    if (!m_authorization_code.empty()) {
        ExchangeCodeForToken(m_authorization_code);
    } else {
        std::cerr << "No authorization code to exchange." << std::endl;
        return 1;
    }

    return 0;
}

void AccessTokenFetcher::SetAuthorizationCode(const std::string& authorization_code) {
    m_authorization_code = authorization_code;
}

std::string AccessTokenFetcher::GetAccessToken() const {
    return m_access_token;
}