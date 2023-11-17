#pragma once
#include <string>

namespace lu::crypto
{
    class Base64EncodeDecode
    {
    public:
        std::string encode(unsigned char *msg, size_t len) const;
        std::pair<unsigned char*, size_t> decode(const std::string& data, size_t len) const;
    };
}