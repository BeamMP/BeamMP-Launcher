/*
 Copyright (C) 2026 BeamMP Ltd., BeamMP team and contributors.
 Licensed under AGPL-3.0 (or later), see <https://www.gnu.org/licenses/>.
 SPDX-License-Identifier: AGPL-3.0-or-later
*/

#include "Jobs/ZipReader.h"

#include <algorithm>
#include <cstring>
#include <vector>
#include <zlib.h>

namespace Jobs {

namespace {
    uint16_t U16(const unsigned char* p) { return uint16_t(p[0] | (p[1] << 8)); }
    uint32_t U32(const unsigned char* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
    uint64_t U64(const unsigned char* p) { return uint64_t(U32(p)) | (uint64_t(U32(p + 4)) << 32); }
}

ZipReader::ZipReader(const std::filesystem::path& path)
    : mFile(path, std::ios::binary) {
    if (!mFile)
        return;
    mFile.seekg(0, std::ios::end);
    const uint64_t size = uint64_t(mFile.tellg());
    if (size < 22)
        return;
    // end of central directory record: last 22 bytes + up to 64 KiB comment
    const uint64_t tail = std::min<uint64_t>(size, 22 + 65535 + 20);
    std::vector<unsigned char> buf(tail);
    mFile.seekg(std::streamoff(size - tail));
    mFile.read(reinterpret_cast<char*>(buf.data()), std::streamsize(tail));
    if (!mFile)
        return;
    int64_t eocd = -1;
    for (int64_t i = int64_t(tail) - 22; i >= 0; i--) {
        if (U32(&buf[size_t(i)]) == 0x06054b50) {
            eocd = i;
            break;
        }
    }
    if (eocd < 0)
        return;
    const unsigned char* e = &buf[size_t(eocd)];
    mCount = U16(e + 10);
    mCdSize = U32(e + 12);
    mCdOffset = U32(e + 16);
    // zip64: locator right before the record points at the zip64 end of central directory
    if (eocd >= 20 && U32(&buf[size_t(eocd - 20)]) == 0x07064b50) {
        const uint64_t z64 = U64(&buf[size_t(eocd - 20) + 8]);
        unsigned char r[56];
        mFile.seekg(std::streamoff(z64));
        mFile.read(reinterpret_cast<char*>(r), sizeof(r));
        if (mFile && U32(r) == 0x06064b50) {
            mCount = U64(r + 32);
            mCdSize = U64(r + 40);
            mCdOffset = U64(r + 48);
        }
    }
    mOk = mCdOffset + mCdSize <= size;
}

void ZipReader::ForEach(const std::function<void(const Entry&)>& fn) {
    if (!mOk)
        return;
    std::vector<unsigned char> cd(static_cast<size_t>(mCdSize));
    mFile.clear();
    mFile.seekg(std::streamoff(mCdOffset));
    mFile.read(reinterpret_cast<char*>(cd.data()), std::streamsize(cd.size()));
    if (!mFile)
        return;
    size_t p = 0;
    for (uint64_t n = 0; n < mCount && p + 46 <= cd.size(); n++) {
        const unsigned char* h = &cd[p];
        if (U32(h) != 0x02014b50)
            break;
        Entry e;
        e.method = U16(h + 10);
        e.compressedSize = U32(h + 20);
        e.size = U32(h + 24);
        const uint16_t nameLen = U16(h + 28), extraLen = U16(h + 30), commentLen = U16(h + 32);
        e.localHeaderOffset = U32(h + 42);
        if (p + 46 + nameLen + extraLen > cd.size())
            break;
        e.name.assign(reinterpret_cast<const char*>(h + 46), nameLen);
        // zip64 extra field: only the values that are 0xFFFFFFFF above, in this order
        for (size_t x = 0; x + 4 <= extraLen;) {
            const unsigned char* f = h + 46 + nameLen + x;
            const uint16_t id = U16(f), len = U16(f + 2);
            if (id == 0x0001) {
                size_t q = 4;
                if (e.size == 0xFFFFFFFF && q + 8 <= 4u + len) { e.size = U64(f + q); q += 8; }
                if (e.compressedSize == 0xFFFFFFFF && q + 8 <= 4u + len) { e.compressedSize = U64(f + q); q += 8; }
                if (e.localHeaderOffset == 0xFFFFFFFF && q + 8 <= 4u + len) { e.localHeaderOffset = U64(f + q); }
            }
            x += 4 + len;
        }
        p += 46 + nameLen + extraLen + commentLen;
        if (!e.name.empty() && e.name.back() != '/')
            fn(e);
    }
}

std::optional<std::string> ZipReader::Read(const Entry& e) {
    if (!mOk || (e.method != 0 && e.method != 8) || e.size > (1ull << 31))
        return std::nullopt;
    unsigned char lh[30];
    mFile.clear();
    mFile.seekg(std::streamoff(e.localHeaderOffset));
    mFile.read(reinterpret_cast<char*>(lh), sizeof(lh));
    if (!mFile || U32(lh) != 0x04034b50)
        return std::nullopt;
    mFile.seekg(std::streamoff(e.localHeaderOffset + 30 + U16(lh + 26) + U16(lh + 28)));
    std::string packed(size_t(e.compressedSize), '\0');
    mFile.read(packed.data(), std::streamsize(packed.size()));
    if (!mFile)
        return std::nullopt;
    if (e.method == 0)
        return packed;
    std::string out(size_t(e.size), '\0');
    z_stream zs {};
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) // raw deflate
        return std::nullopt;
    zs.next_in = reinterpret_cast<Bytef*>(packed.data());
    zs.avail_in = uInt(packed.size());
    zs.next_out = reinterpret_cast<Bytef*>(out.data());
    zs.avail_out = uInt(out.size());
    const int res = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (res != Z_STREAM_END || zs.total_out != out.size())
        return std::nullopt;
    return out;
}

}
