/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
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
};