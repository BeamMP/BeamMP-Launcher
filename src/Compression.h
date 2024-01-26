#pragma once

#include <array>
#include <zlib.h>
#include <algorithm>

#define Biggest 30000

template <typename T>
inline T Comp(const T& Data) {
    std::array<char, Biggest> C {};
    // obsolete
    C.fill(0);
    z_stream defstream;
    defstream.zalloc = nullptr;
    defstream.zfree = nullptr;
    defstream.opaque = nullptr;
    defstream.avail_in = uInt(Data.size());
    defstream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(&Data[0]));
    defstream.avail_out = Biggest;
    defstream.next_out = reinterpret_cast<Bytef*>(C.data());
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_SYNC_FLUSH);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);
    size_t TotalOut = defstream.total_out;
    T Ret;
    Ret.resize(TotalOut);
    std::fill(Ret.begin(), Ret.end(), 0);
    std::copy_n(C.begin(), TotalOut, Ret.begin());
    return Ret;
}

template <typename T>
inline T DeComp(const T& Compressed) {
    std::array<char, Biggest> C {};
    // not needed
    C.fill(0);
    z_stream infstream;
    infstream.zalloc = nullptr;
    infstream.zfree = nullptr;
    infstream.opaque = nullptr;
    infstream.avail_in = Biggest;
    infstream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(&Compressed[0]));
    infstream.avail_out = Biggest;
    infstream.next_out = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(C.data()));
    inflateInit(&infstream);
    inflate(&infstream, Z_SYNC_FLUSH);
    inflate(&infstream, Z_FINISH);
    inflateEnd(&infstream);
    size_t TotalOut = infstream.total_out;
    T Ret;
    Ret.resize(TotalOut);
    std::fill(Ret.begin(), Ret.end(), 0);
    std::copy_n(C.begin(), TotalOut, Ret.begin());
    return Ret;
}