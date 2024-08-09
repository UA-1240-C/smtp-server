#include "Base64.h"

namespace ISXBase64
{
std::string Base64Encode(const std::string& decoded)
{
  // Calculate the size needed for the encoded output
  const auto encoded_size = boost::beast::detail::base64::encoded_size(decoded.size());

  // Prepare the output string with the required size and initialize it with null characters
  std::string encoded_output(encoded_size, '\0');

  // Perform the encoding
  boost::beast::detail::base64::encode(encoded_output.data(), decoded.data(), decoded.size());

  // Return the Base64 encoded string
  return encoded_output;
}

std::string Base64Decode(const std::string& encoded) {
  // Calculate the size needed for the decoded output
  const auto decoded_size = boost::beast::detail::base64::decoded_size(encoded.size());
  std::string decoded_output(decoded_size, '\0');

  // Decode the Base64 encoded string
  const auto [decoded_length, success] = boost::beast::detail::base64::decode(
      decoded_output.data(), encoded.data(), encoded.size()
  );

  // Resize the decoded output to the actual length of decoded data
  decoded_output.resize(decoded_length);

  return decoded_output;
}
}; // namespace ISXBase64