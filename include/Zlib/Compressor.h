/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once
#include <span>
#include <vector>

std::vector<char> Comp(std::span<const char> input);
std::vector<char> DeComp(std::span<const char> input);
