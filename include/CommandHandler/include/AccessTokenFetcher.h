#ifndef ACCESSTOKENFETCHER_H
#define ACCESSTOKENFETCHER_H

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <json/json.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;

class AccessTokenFetcher {
public:
    AccessTokenFetcher(std::string client_id, std::string client_secret, std::string authorization_code, std::string redirect_uri);

    std::string FetchAccessToken();

private:
    std::string client_id_;
    std::string client_secret_;
    std::string authorization_code_;
    std::string redirect_uri_;
    asio::io_context ioc_;
    ssl::context ctx_;
    ssl::stream<tcp::socket> stream_;

    std::string UrlEncode(const std::string& value);
    void SendRequest();
    std::string ProcessResponse();
    std::string ParseAccessToken(const std::string& json_response);
};

#endif //ACCESSTOKENFETCHER_H
