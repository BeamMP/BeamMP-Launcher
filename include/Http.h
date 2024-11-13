/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include "Logger.h"
#include <string>
class HTTP {
public:
    static bool Download(const std::string& IP, const std::wstring& Path);
    static std::string Post(const std::string& IP, const std::string& Fields);
    static std::string Get(const std::string& IP);
    static bool ProgressBar(size_t c, size_t t);
    static void StartProxy();
public:
    static bool isDownload;
};
