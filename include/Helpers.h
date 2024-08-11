#include <span>
#include <string>
#include <vector>

#pragma once

using ByteSpan = std::span<const char>;

std::string bytespan_to_string(ByteSpan span);

std::vector<char> strtovec(std::string_view str);
