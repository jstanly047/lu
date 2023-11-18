#pragma once
#include <crypto/DataWrap.h>
#include <string>

namespace lu::crypto
{
    class Base64EncodeDecode
    {
    public:
        std::string encode(DataWrap& data) const;
        DataWrap decode(const std::string& data, int len) const;
    };
}