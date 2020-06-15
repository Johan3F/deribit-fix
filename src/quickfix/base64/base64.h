#pragma once

#include <string>

namespace Deribit {

std::string base64_encode(unsigned char const*, unsigned int len);
std::string base64_decode(std::string const& s);

}  // namespace Deribit
