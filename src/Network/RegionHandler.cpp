/*
 Copyright (C) 2024 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "RegionHandler.h"
#include "Logger.h"
#include "Options.h"

#include <regex>

void RegionHandler::TopLevelDomainFailed()
{
    info("Top level domain of " + mValidTLDs[mRegionIndex % mValidTLDs.size()] + " didn't respond correctly , changing domain to " + mValidTLDs[(mRegionIndex + 1) % mValidTLDs.size()]);
    mRegionIndex++;
}

std::string RegionHandler::RegionToTopLevelDomain()
{
    static bool isDeveloperRegion = options.region == "Developer";
    if (isDeveloperRegion) {
        return "beammp.dev";
    }
    return mValidTLDs[mRegionIndex % mValidTLDs.size()]; // Global
}

std::string RegionHandler::RedirectURL(const std::string &URL)
{
    std::regex link_pattern(R"(^(https:\/\/.*)beammp\.com(\/.*)?$)");
    std::smatch link_match;
    if (std::regex_search(URL, link_match, link_pattern) && link_match.position() == 0) {
        //TLD matched beammp.com
        std::string before = link_match[1].str();                     // "https://..." up to beammp
        std::string after  = link_match[2].matched ? link_match[2].str() : ""; // "/path" or ""
        return before + RegionToTopLevelDomain() + after;
    }
    return URL; //if it didn't match, just return the unmodified URL
}
