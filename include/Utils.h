/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <cassert>
#include <filesystem>
#include <fstream>
#include <locale>
#include <map>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <regex>
#include <string>
#include <vector>

#ifdef _WIN32
#define beammp_fs_string std::wstring
#define beammp_fs_char wchar_t
#define beammp_wide(str) L##str
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
    inline std::string ExpandEnvVars(const std::string& input) {
        std::string result;
        std::regex envPattern(R"(%([^%]+)%|\$([A-Za-z_][A-Za-z0-9_]*)|\$\{([^}]+)\})");

        std::sregex_iterator begin(input.begin(), input.end(), envPattern);
        std::sregex_iterator end;

        size_t lastPos = 0;

        for (auto it = begin; it != end; ++it) {
            const auto& match = *it;

            result.append(input, lastPos, match.position() - lastPos);

            std::string varName;
            if (match[1].matched) varName = match[1].str(); // %VAR%
            else if (match[2].matched) varName = match[2].str(); // $VAR
            else if (match[3].matched) varName = match[3].str(); // ${VAR}

            if (const char* envValue = std::getenv(varName.c_str())) {
                result.append(envValue);
            }

            lastPos = match.position() + match.length();
        }

        result.append(input, lastPos, input.length() - lastPos);

        return result;
    }
#ifdef _WIN32
    inline std::wstring ExpandEnvVars(const std::wstring& input) {
        std::wstring result;
        std::wregex envPattern(LR"(%([^%]+)%|\$([A-Za-z_][A-Za-z0-9_]*)|\$\{([^}]+)\})");

        std::wsregex_iterator begin(input.begin(), input.end(), envPattern);
        std::wsregex_iterator end;

        size_t lastPos = 0;

        for (auto it = begin; it != end; ++it) {
            const auto& match = *it;

            result.append(input, lastPos, match.position() - lastPos);

            std::wstring varName;
            assert(match.size() == 4 && "Input regex has incorrect amount of capturing groups");
            if (match[1].matched) varName = match[1].str(); // %VAR%
            else if (match[2].matched) varName = match[2].str(); // $VAR
            else if (match[3].matched) varName = match[3].str(); // ${VAR}

            if (const wchar_t* envValue = _wgetenv(varName.c_str())) {
                if (envValue != nullptr) {
                    result.append(envValue);
                }
            }

            lastPos = match.position() + match.length();
        }

        result.append(input, lastPos, input.length() - lastPos);

        return result;
    }
#endif
    inline std::map<std::string, std::map<std::string, std::string>> ParseINI(const std::string& contents) {
        std::map<std::string, std::map<std::string, std::string>> ini;

        std::string currentSection;
        auto sections = Split(contents, "\n");

        for (size_t i = 0; i < sections.size(); i++) {
            std::string line = sections[i];
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            for (auto& c : line) {
                if (c == '#' || c == ';') {
                    line = line.substr(0, &c - &line[0]);
                    break;
                }
            }

            auto invalidLineLog = [&]{
                warn("Invalid INI line: " + line);
                warn("Surrounding lines: \n" +
                    (i > 0 ? sections[i - 1] : "") + "\n" +
                    (i < sections.size() - 1 ? sections[i + 1] : ""));
            };

            if (line[0] == '[') {
                currentSection = line.substr(1, line.find(']') - 1);
            } else {

                if (currentSection.empty()) {
                    invalidLineLog();
                    continue;
                }

                std::string key, value;
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    key = line.substr(0, pos);
                    value = line.substr(pos + 1);
                    ini[currentSection][key] = value;
                } else {
                    invalidLineLog();
                    continue;
                }

                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                ini[currentSection][key] = value;
            }
        }

        return ini;
    }

#ifdef _WIN32
inline std::wstring ToWString(const std::string& s) {
        if (s.empty()) return std::wstring();

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        if (size_needed <= 0) {
            return L"";
        }

        std::wstring result(size_needed, 0);

        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &result[0], size_needed);

        return result;
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

            std::ifstream stream(filename, std::ios::binary);

            const size_t FileSize = std::filesystem::file_size(filename);
            size_t Read = 0;
            std::vector<char> Data;
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