#include "Base64.h"

namespace ISXBase64
{
  string Base64Encode(const string& decoded)
  {
    auto encoded_size = boost::beast::detail::base64::encoded_size(decoded.size());
    std::string encoded_output(encoded_size, '\0');
    boost::beast::detail::base64::encode(encoded_output.data(), decoded.data(), decoded.size());
    return encoded_output;
  };

  string Base64Decode(const string& encoded)
  {
    auto decoded_size = boost::beast::detail::base64::decoded_size(encoded.size());
    std::string decoded_output(decoded_size, '\0');

    auto result = boost::beast::detail::base64::decode(decoded_output.data(), encoded.data(), encoded.size());

    decoded_output.resize(result.first);
    return decoded_output;
  };
}; // namespace ISXBase64