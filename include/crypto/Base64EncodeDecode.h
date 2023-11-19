#pragma once
#include <crypto/DataWrap.h>
#include <string>

namespace lu::crypto
{
    class Base64EncodeDecode
    {
    public:
        static std::string encode(DataWrap& data) ;
        static DataWrap decode(const std::string& b64message, int len) ;
    };
}