/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <compare>
#include <string>
#include <vector>

void InitLauncher();
std::wstring GetEP(const wchar_t* P = nullptr);
std::wstring GetGamePath();
std::string GetVer();
std::string GetPatch();
std::wstring GetEN();
void ConfigInit();
