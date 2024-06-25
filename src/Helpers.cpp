#include "Helpers.h"

std::string bytespan_to_string(ByteSpan span) {
    return std::string(span.data(), span.size());
}

std::vector<char> strtovec(std::string_view str) {
    return std::vector<char>(str.begin(), str.end());
}
