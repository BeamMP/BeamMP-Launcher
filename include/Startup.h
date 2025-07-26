/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include "Utils.h"

#include <compare>
#include <string>
#include <vector>

void InitLauncher();
beammp_fs_string GetEP(const beammp_fs_char* P = nullptr);
std::filesystem::path GetBP(const beammp_fs_char* P = nullptr);
std::filesystem::path GetGamePath();
std::string GetVer();
std::string GetPatch();
beammp_fs_string GetEN();
void ConfigInit();
