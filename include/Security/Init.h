/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <string>
void PreGame(const beammp_fs_string& GamePath);
std::string CheckVer(const beammp_fs_string& path);
void InitGame(const beammp_fs_string& Dir);
beammp_fs_string GetGameDir();
void LegitimacyCheck();
void CheckLocalKey();