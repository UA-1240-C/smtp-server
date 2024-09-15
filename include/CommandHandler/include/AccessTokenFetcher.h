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
    AccessTokenFetcher();
    ~AccessTokenFetcher();

    int FetchAccessToken();
    void SetAuthorizationCode(const std::string& authorization_code);
    std::string GetAccessToken() const;

private:
    std::string m_authorization_code;
    std::string m_access_token;

    std::string UrlEncode(const std::string& value) const;
    void HandleRequest(tcp::socket& socket);
    int OpenOAuthBrowser() const;
    std::string ParseAccessToken(const std::string& json_response) const;
    void ExchangeCodeForToken(const std::string& authorization_code);
};

#endif //ACCESSTOKENFETCHER_H
