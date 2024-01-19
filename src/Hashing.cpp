#include "Hashing.h"

#include <boost/iostreams/device/mapped_file.hpp>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <cstdio>
#include <filesystem>
#include <fmt/format.h>
#include <errno.h>

std::string sha256_file(const std::string& path) {
    CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
    digest[sizeof(digest) - 1] = 0;
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        throw std::runtime_error(fmt::format("Failed to open {}: {}", path, std::strerror(errno)));
    }
    std::vector<uint8_t> buffer{};
    buffer.resize(std::filesystem::file_size(path));
    std::fread(buffer.data(), 1, buffer.size(), file);
    std::fclose(file);
    CryptoPP::SHA256().CalculateDigest(
        digest,
        buffer.data(), buffer.size());
    CryptoPP::HexEncoder encoder;
    encoder.Put(digest, sizeof(digest));
    encoder.MessageEnd();
    std::string encoded {};
    if (auto size = encoder.MaxRetrievable(); size != 0)
    {
        encoded.resize(size);
        encoder.Get(reinterpret_cast<CryptoPP::byte*>(&encoded[0]), encoded.size());
    }
    return encoded;
}
