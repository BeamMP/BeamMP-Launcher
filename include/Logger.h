/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <iostream>
#include <string>
void InitLog();
void except(const std::string& toPrint);
void fatal(const std::string& toPrint);
void debug(const std::string& toPrint);
void error(const std::string& toPrint);
void info(const std::string& toPrint);
void warn(const std::string& toPrint);

void except(const std::wstring& toPrint);
void fatal(const std::wstring& toPrint);
void debug(const std::wstring& toPrint);
void error(const std::wstring& toPrint);
void info(const std::wstring& toPrint);
void warn(const std::wstring& toPrint);
std::string getDate();
