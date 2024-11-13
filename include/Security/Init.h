/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <string>
void PreGame(const std::wstring& GamePath);
std::string CheckVer(const std::wstring& path);
void InitGame(const std::wstring& Dir);
std::wstring GetGameDir();
void LegitimacyCheck();
void CheckLocalKey();