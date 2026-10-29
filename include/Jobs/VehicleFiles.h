/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once

// Read only view of the game's /vehicles/ tree, merged the way the game mounts it:
// game content zips, then mod zips of the user folder, then unpacked mods, then loose files in the
// user folder, later sources replacing earlier ones. Paths are game style ("/vehicles/pickup/pickup.jbeam"),
// matched without case.

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Jobs {

class VehicleFiles {
public:
    VehicleFiles(std::filesystem::path gameDir, std::filesystem::path userDir);
    ~VehicleFiles();

    // virtual paths below dir (game style, ends with '/') whose name ends with suffix (lower case, e.g. ".jbeam")
    std::vector<std::string> List(const std::string& dir, const std::string& suffix) const;
    std::optional<std::string> Read(const std::string& path) const;
    bool Exists(const std::string& path) const;
    // a string that changes when any file below dir changes source, size or time (cache validation)
    std::string Signature(const std::string& dir) const;
    size_t FileCount() const;
    const std::filesystem::path& UserDir() const { return mUserDir; }

    // shared instance for the current game + user folder, rebuilt when the mounted sources change
    static std::shared_ptr<VehicleFiles> Get(const std::filesystem::path& gameDir, const std::filesystem::path& userDir);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
    std::filesystem::path mUserDir;
};

// lower case, forward slashes, leading '/'
std::string NormalizeGamePath(std::string p);

}
