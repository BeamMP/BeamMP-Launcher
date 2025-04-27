/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <map>
#include <regex>
#include <string>
#include <vector>

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

};