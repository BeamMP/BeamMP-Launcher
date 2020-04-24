///
/// Created by Anonymous275 on 4/23/2020
///

#include <string>
#include <iostream>
#include "include/zlib.h"


void Print(const std::string&MSG){
    //std::cout << MSG << std::endl;
}


std::string Compress(const std::string&Data){
    std::string b;
    b.resize(Data.size());
    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    defstream.avail_in = (uInt)Data.length()+1;
    defstream.next_in = (Bytef *)&Data[0];
    defstream.avail_out = (uInt)b.size();
    defstream.next_out = (Bytef *)&b[0];
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);
    for(int i = int(b.length())-1;i >= 0;i--){
        if(b.at(i) != '\0'){
            b.resize(i);
            break;
        }
    }
    return b;
}

std::string Decompress(const std::string&Data)
{
    std::string c;
    c.resize(Data.size()*5);
    z_stream infstream,defstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt)((char*)defstream.next_out - Data.c_str());
    infstream.next_in = (Bytef *)&Data[0];
    infstream.avail_out = (uInt)c.size();
    infstream.next_out = (Bytef *)&c[0];
    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    for(int i = int(c.length())-1;i >= 0;i--){
        if(c.at(i) != '\0'){
            c.resize(i+2);
            break;
        }
    }
    return c;
}