#pragma once

#include <cstdint>
#include <array>
#include <string>

struct Version {
    uint8_t major {};
    uint8_t minor {};
    uint8_t patch {};
    Version() {}
    Version(uint8_t major, uint8_t minor, uint8_t patch);
    Version(const std::array<uint8_t, 3>& v);
    explicit Version(const std::string& str);
    std::string to_string();
    bool is_outdated(const Version& new_version);
};
