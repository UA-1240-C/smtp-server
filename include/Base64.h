#ifndef BASE64_H
#define BASE64_H

#pragma once

#include <boost/beast/core/detail/base64.hpp>
#include <string>

using std::string;

namespace ISXBase64
{
  string Base64Encode(const string& decoded);

  string Base64Decode(const string& encoded);
}; // namespace ISXBase64

#endif //BASE64_H
