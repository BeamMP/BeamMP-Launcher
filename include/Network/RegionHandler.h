/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once

#include <vector>
#include <string>
#include <array>

class RegionHandler final {
public:
    RegionHandler() = delete;
    static void TopLevelDomainFailed(bool failed);
    static std::string RegionToTopLevelDomain(const std::string region);
private:
    static inline unsigned int mRegionIndex { 0 };
    const static inline std::array<std::string, 2> mValidTLDs {"beammp.com", "beammp.ru"};
};
