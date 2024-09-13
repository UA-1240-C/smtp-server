#include <utility>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "AccessTokenFetcher.h"

AccessTokenFetcher::AccessTokenFetcher(std::string client_id, std::string client_secret, std::string authorization_code, std::string redirect_uri)
        : client_id_(std::move(client_id))
        , client_secret_(std::move(client_secret))
        , authorization_code_(std::move(authorization_code))
        , redirect_uri_(std::move(redirect_uri))
        , ioc_()
        , ctx_(ssl::context::tls_client)
        , stream_(ioc_, ctx_) {}

std::string AccessTokenFetcher::FetchAccessToken() {
    try {
        // Set up the SSL context
        ctx_.set_default_verify_paths();

        // Resolve the server
        tcp::resolver resolver{ioc_};
        auto const results = resolver.resolve("oauth2.googleapis.com", "https");
        asio::connect(stream_.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        stream_.handshake(ssl::stream_base::client);

        // Construct and send the POST request
        SendRequest();

        // Receive and process the response
        return ProcessResponse();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
}

std::string AccessTokenFetcher::UrlEncode(const std::string& value) {
    std::ostringstream out;
    for (char c : value) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out << c;
        } else {
            out << '%' << std::hex << std::uppercase << int(static_cast<unsigned char>(c));
        }
    }
    return out.str();
}

void AccessTokenFetcher::SendRequest() {
    std::string body = "client_id=" + UrlEncode(client_id_) +
                       "&client_secret=" + UrlEncode(client_secret_) +
                       "&code=" + UrlEncode(authorization_code_) +
                       "&grant_type=" + UrlEncode("authorization_code") +
                       "&redirect_uri=" + UrlEncode(redirect_uri_);

    // Set up the POST request
    http::request<http::string_body> req{http::verb::post, "/token", 11};
    req.set(http::field::host, "oauth2.googleapis.com");
    req.set(http::field::content_type, "application/x-www-form-urlencoded");
    req.body() = body;
    req.prepare_payload();

    // Send the request
    http::write(stream_, req);
}

std::string AccessTokenFetcher::ParseAccessToken(const std::string& json_response) {
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

std::string AccessTokenFetcher::ProcessResponse() {
    // Receive the response
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(stream_, buffer, res);

    // Convert the response body to a string
    std::string response_body = buffers_to_string(res.body().data());

    // Print the response body (optional)
    std::cout << "Response Body: " << response_body << std::endl;

    // Parse and return the access token from the response
    return ParseAccessToken(response_body);
}
