/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "RegionHandler.h"
#include "Logger.h"

void RegionHandler::TopLevelDomainFailed(bool failed)
{
    if (!failed) return;
    info("Top level domain of " + mValidTLDs[mRegionIndex % mValidTLDs.size()] + " didn't respond correctly , changing domain to " + mValidTLDs[(mRegionIndex + 1) % mValidTLDs.size()]);
    mRegionIndex++;
}

std::string RegionHandler::RegionToTopLevelDomain(const std::string region)
{
    if (region == "Developer") {
        return "beammp.dev";
    }
    return mValidTLDs[mRegionIndex % mValidTLDs.size()]; // Global
}
