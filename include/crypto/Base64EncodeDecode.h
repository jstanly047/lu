#pragma once
#include <crypto/DataWrap.h>
#include <openssl/types.h>
#include <string>
#include <openssl/pem.h>

namespace lu::crypto
{
    class Base64EncodeDecode
    {
    public:
        static std::string encode(DataWrap& data) ;
        static void encode(void* ptr, int size, void* dest, std::size_t maxLength);
        static DataWrap decode(const std::string& b64message) ;

    private:
        static ::BIO* b64Encoder();
        static ::BIO *bio;
    };
}