/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#pragma once

// Minimal read only zip access (stored and deflated entries, zip64) on top of zlib, for the job workers.
// Unlike zip_file.h it never loads the whole archive into memory (game content zips are hundreds of MB),
// and each instance is independent, so worker threads can read at the same time.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>

namespace Jobs {

class ZipReader {
public:
    struct Entry {
        std::string name;
        uint64_t compressedSize, size, localHeaderOffset;
        uint16_t method;
    };

    explicit ZipReader(const std::filesystem::path& path);
    bool IsOpen() const { return mOk; }
    // calls fn for every file entry of the central directory
    void ForEach(const std::function<void(const Entry&)>& fn);
    std::optional<std::string> Read(const Entry& e);

private:
    std::ifstream mFile;
    uint64_t mCdOffset = 0, mCdSize = 0, mCount = 0;
    bool mOk = false;
};

}
