#include "Version.h"
#include <charconv>
#include <fmt/format.h>
#include <sstream>

Version::Version(uint8_t major, uint8_t minor, uint8_t patch)
    : major(major)
    , minor(minor)
    , patch(patch) { }

Version::Version(const std::array<uint8_t, 3>& v)
    : Version(v[0], v[1], v[2]) {
}

std::string Version::to_string() {
    return fmt::format("{:d}.{:d}.{:d}", major, minor, patch);
}

bool Version::is_outdated(const Version& new_version) {
    if (new_version.major > major) {
        return true;
    } else if (new_version.major == major && new_version.minor > minor) {
        return true;
    } else if (new_version.major == major && new_version.minor == minor && new_version.patch > patch) {
        return true;
    } else {
        return false;
    }
}
Version::Version(const std::string& str) {
    std::stringstream ss(str);
    std::string Part;
    std::getline(ss, Part, '.');
    std::from_chars(&*Part.begin(), &*Part.begin() + Part.size(), major);
    std::from_chars(&*Part.begin(), &*Part.begin() + Part.size(), minor);
    std::from_chars(&*Part.begin(), &*Part.begin() + Part.size(), patch);
}
