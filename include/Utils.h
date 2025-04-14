/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <filesystem>
#include <fstream>
#include <locale>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define beammp_fs_string std::wstring
#define beammp_fs_char wchar_t
#define beammp_wide(str) L##str
#else
#define beammp_fs_string std::string
#define beammp_fs_char char
#define beammp_wide(str) str
#endif

namespace Utils {
    inline std::vector<std::string> Split(const std::string& String, const std::string& delimiter) {
        std::vector<std::string> Val;
        size_t pos;
        std::string token, s = String;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            if (!token.empty())
                Val.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        if (!s.empty())
            Val.push_back(s);
        return Val;
    };
#ifdef _WIN32
    inline std::wstring ToWString(const std::string& s) {
        return std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>>().from_bytes(s);
    }
#else
    inline std::string ToWString(const std::string& s) {
        return s;
    }
#endif
    inline std::string GetSha256HashReallyFast(const beammp_fs_string& filename) {
        try {
            EVP_MD_CTX* mdctx;
            const EVP_MD* md;
            uint8_t sha256_value[EVP_MAX_MD_SIZE];
            md = EVP_sha256();
            if (md == nullptr) {
                throw std::runtime_error("EVP_sha256() failed");
            }

            mdctx = EVP_MD_CTX_new();
            if (mdctx == nullptr) {
                throw std::runtime_error("EVP_MD_CTX_new() failed");
            }
            if (!EVP_DigestInit_ex2(mdctx, md, NULL)) {
                EVP_MD_CTX_free(mdctx);
                throw std::runtime_error("EVP_DigestInit_ex2() failed");
            }

            std::wifstream stream(filename, std::ios::binary);

            const size_t FileSize = std::filesystem::file_size(filename);
            size_t Read = 0;
            std::vector<wchar_t> Data;
            while (Read < FileSize) {
                Data.resize(size_t(std::min<size_t>(FileSize - Read, 4096)));
                size_t RealDataSize = Data.size();
                stream.read(Data.data(), std::streamsize(Data.size()));
                if (stream.eof() || stream.fail()) {
                    RealDataSize = size_t(stream.gcount());
                }
                Data.resize(RealDataSize);
                if (RealDataSize == 0) {
                    break;
                }
                if (RealDataSize > 0 && !EVP_DigestUpdate(mdctx, Data.data(), Data.size())) {
                    EVP_MD_CTX_free(mdctx);
                    throw std::runtime_error("EVP_DigestUpdate() failed");
                }
                Read += RealDataSize;
            }
            unsigned int sha256_len = 0;
            if (!EVP_DigestFinal_ex(mdctx, sha256_value, &sha256_len)) {
                EVP_MD_CTX_free(mdctx);
                throw std::runtime_error("EVP_DigestFinal_ex() failed");
            }
            EVP_MD_CTX_free(mdctx);

            std::string result;
            for (size_t i = 0; i < sha256_len; i++) {
                char buf[3];
                sprintf(buf, "%02x", sha256_value[i]);
                buf[2] = 0;
                result += buf;
            }
            return result;
        } catch (const std::exception& e) {
            error(beammp_wide("Sha256 hashing of '") + filename + beammp_wide("' failed: ") + ToWString(e.what()));
            return "";
        }
    }
};