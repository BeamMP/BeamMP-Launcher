// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/15/2020
///

#include "Logger.h"
#include <span>
#include <vector>
#include <zconf.h>
#include <zlib.h>
#ifdef __linux__
#include <cstring>
#endif

std::vector<char> Comp(std::span<const char> input) {
    auto max_size = compressBound(input.size());
    std::vector<char> output(max_size);
    uLongf output_size = output.size();
    int res = compress(
        reinterpret_cast<Bytef*>(output.data()),
        &output_size,
        reinterpret_cast<const Bytef*>(input.data()),
        static_cast<uLongf>(input.size()));
    if (res != Z_OK) {
        error("zlib compress() failed (code: " + std::to_string(res) + ", message: " + zError(res) + ")");
        throw std::runtime_error("zlib compress() failed");
    }
    debug("zlib compressed " + std::to_string(input.size()) + " B to " + std::to_string(output_size) + " B");
    output.resize(output_size);
    return output;
}

std::vector<char> DeComp(std::span<const char> input) {
    std::vector<char> output_buffer(std::min<size_t>(input.size() * 5, 15 * 1024 * 1024));

    uLongf output_size = output_buffer.size();

    while (true) {
        int res = uncompress(
            reinterpret_cast<Bytef*>(output_buffer.data()),
            &output_size,
            reinterpret_cast<const Bytef*>(input.data()),
            static_cast<uLongf>(input.size()));
        if (res == Z_BUF_ERROR) {
            if (output_buffer.size() > 30 * 1024 * 1024) {
                throw std::runtime_error("decompressed packet size of 30 MB exceeded");
            }
            debug("zlib uncompress() failed, trying with 2x buffer size of " + std::to_string(output_buffer.size() * 2));
            output_buffer.resize(output_buffer.size() * 2);
            output_size = output_buffer.size();
        } else if (res != Z_OK) {
            error("zlib uncompress() failed (code: " + std::to_string(res) + ", message: " + zError(res) + ")");
            throw std::runtime_error("zlib uncompress() failed");
        } else if (res == Z_OK) {
            break;
        }
    }    output_buffer.resize(output_size);
    return output_buffer;
}
