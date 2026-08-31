/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include "Logger.h"
#include "Utils.h"

#include <string>
class HTTP {
public:
    static bool Download(const std::string& IP, const beammp_fs_string& Path, const std::string& Hash, const bool& redirect = true);
    static std::string Post(std::string IP, const std::string& Fields, const bool& redirect = true);
    static std::string Get(std::string IP, const bool& redirect = true);
    static bool ProgressBar(size_t c, size_t t);
    static void StartProxy();
public:
    static bool isDownload;
};
