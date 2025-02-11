/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <string>
void PreGame(const std::string& GamePath);
std::string CheckVer(const std::string& path);
void InitGame(const std::string& Dir);
std::string GetGameDir();
#if defined(__APPLE__) 
std::string GetBottlePath();
std::string GetBottleName();
#endif
void LegitimacyCheck();
void CheckLocalKey();